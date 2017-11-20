#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "CookieJar.h"
#include "DownloadManager.h"
#include "FaviconStorage.h"
#include "HistoryManager.h"
#include "HistoryWidget.h"
#include "MainWindow.h"
#include "SearchEngineManager.h"
#include "Settings.h"
#include "NetworkAccessManager.h"
#include "URLSuggestionModel.h"
#include "UserAgentManager.h"
#include "WebPage.h"

#include <QDir>
#include <QUrl>
#include <QDebug>

BrowserApplication::BrowserApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName(QString("Vaccarelli"));
    QCoreApplication::setApplicationName(QString("Viper Browser"));
    QCoreApplication::setApplicationVersion(QString("0.2"));

    setAttribute(Qt::AA_DontShowIconsInMenus, false);

    // Load settings
    m_settings = std::make_shared<Settings>();

    // Initialize bookmarks manager
    m_bookmarks = DatabaseWorker::createWorker<BookmarkManager>(m_settings->firstRun(), m_settings->getPathValue("BookmarkPath"));

    // Initialize cookie jar
    m_cookieJar = DatabaseWorker::createWorker<CookieJar>(m_settings->firstRun(), m_settings->getPathValue("CookiePath"));

    // Initialize download manager
    m_downloadMgr = new DownloadManager;
    m_downloadMgr->setDownloadDir(m_settings->getPathValue("DownloadDir"));

    // Initialize favicon storage module
    m_faviconStorage = new FaviconStorage(m_settings->firstRun(), m_settings->getPathValue("FaviconPath"));

    // Instantiate the history manager
    m_historyMgr = new HistoryManager(m_settings->firstRun(), m_settings->getPathValue("HistoryPath"));
    connect(m_historyMgr, &HistoryManager::pageVisited, [=](const QString &url, const QString &title) {
        QUrl itemUrl(url);
        QIcon favicon = m_faviconStorage->getFavicon(itemUrl);

        // update the History menu in each MainWindow
        //QUrl itemUrl = QUrl::fromUserInput(url);
        for (int i = 0; i < m_browserWindows.size(); ++i)
        {
            QPointer<MainWindow> m = m_browserWindows.at(i);
            if (!m.isNull())
                m->addHistoryItem(itemUrl, title, favicon);
        }
    });

    m_historyWidget = nullptr;

    // Create url suggestion model
    m_suggestionModel = new URLSuggestionModel;

    // Create network access managers
    m_networkAccessMgr = new NetworkAccessManager;
    m_networkAccessMgr->setCookieJar(m_cookieJar.get());

    m_downloadMgr->setNetworkAccessManager(m_networkAccessMgr);

    m_privateNetworkAccessMgr = new NetworkAccessManager;
    CookieJar *privateJar = new CookieJar(QString("%1.fake").arg(m_settings->getPathValue("CookiePath")), QString("FakeCookies"), true);
    m_privateNetworkAccessMgr->setCookieJar(privateJar);

    // Setup user agent manager
    m_userAgentMgr = new UserAgentManager(m_settings);
    connect(m_userAgentMgr, &UserAgentManager::updateUserAgents, this, &BrowserApplication::resetUserAgentMenus);

    // Load search engine information
    SearchEngineManager::instance().loadSearchEngines(m_settings->getPathValue("SearchEnginesFile"));

    // Set global web settings
    setWebSettings();

    // TODO: check if argument has been given (argc > 1) and load the resource into a new window
}

BrowserApplication::~BrowserApplication()
{
    for (int i = 0; i < m_browserWindows.size(); ++i)
    {
        QPointer<MainWindow> m = m_browserWindows.at(i);
        if (!m.isNull())
            delete m.data();
    }

    // Prevents network access manager from attempting to delete cookie jar and causing segfault
    m_cookieJar->setParent(nullptr);

    delete m_downloadMgr;
    delete m_faviconStorage;
    delete m_historyMgr;
    delete m_suggestionModel;
    delete m_networkAccessMgr;
    delete m_privateNetworkAccessMgr;
    delete m_historyWidget;
    delete m_userAgentMgr;
}

BrowserApplication *BrowserApplication::instance()
{
    return static_cast<BrowserApplication*>(QCoreApplication::instance());
}

std::shared_ptr<BookmarkManager> BrowserApplication::getBookmarkManager()
{
    return m_bookmarks;
}

std::shared_ptr<CookieJar> BrowserApplication::getCookieJar()
{
    return m_cookieJar;
}

std::shared_ptr<Settings> BrowserApplication::getSettings()
{
    return m_settings;
}

DownloadManager *BrowserApplication::getDownloadManager()
{
    return m_downloadMgr;
}

FaviconStorage *BrowserApplication::getFaviconStorage()
{
    return m_faviconStorage;
}

HistoryManager *BrowserApplication::getHistoryManager()
{
    return m_historyMgr;
}

HistoryWidget *BrowserApplication::getHistoryWidget()
{
    if (!m_historyWidget)
    {
        m_historyWidget = new HistoryWidget;
        m_historyWidget->setHistoryManager(m_historyMgr);
    }

    return m_historyWidget;
}

NetworkAccessManager *BrowserApplication::getNetworkAccessManager()
{
    return m_networkAccessMgr;
}

NetworkAccessManager *BrowserApplication::getPrivateNetworkAccessManager()
{
    return m_privateNetworkAccessMgr;
}

URLSuggestionModel *BrowserApplication::getURLSuggestionModel()
{
    return m_suggestionModel;
}

UserAgentManager *BrowserApplication::getUserAgentManager()
{
    return m_userAgentMgr;
}

MainWindow *BrowserApplication::getNewWindow()
{
    bool firstWindow = m_browserWindows.empty();

    MainWindow *w = new MainWindow(m_settings, m_bookmarks);
    m_browserWindows.append(w);

    // Add recent history to main window
    setHistoryForWindow(w);
    w->show();

    // Check if this is the first window since the application has started - if so, handle
    // the startup mode behavior depending on the user's configuration setting
    if (firstWindow)
    {
        StartupMode mode = m_settings->getValue("StartupMode").value<StartupMode>();
        switch (mode)
        {
            case StartupMode::LoadHomePage:
                w->loadUrl(QUrl::fromUserInput(m_settings->getValue("HomePage").toString()));
                break;
            case StartupMode::LoadBlankPage:
                w->loadBlankPage();
                break;
            case StartupMode::RestoreSession:
                //todo: load previous session data from a JSON file which stores an array of urls,
                //      the method to do this will be in this class (BrowserApplication),
                //      and will require one parameter, the MainWindow pointer
                break;
        }
    }
    else
    {
        // Treat new window as a new tab, and check if new tab behavior is set to
        // open a blank page or load a home page URL
        if (m_settings->getValue("NewTabsLoadHomePage").toBool())
            w->loadUrl(QUrl::fromUserInput(m_settings->getValue("HomePage").toString()));
        else
            w->loadBlankPage();
    }

    return w;
}

MainWindow *BrowserApplication::getNewPrivateWindow()
{
    MainWindow *w = new MainWindow(m_settings, m_bookmarks);
    m_browserWindows.append(w);
    setHistoryForWindow(w);

    // Set to private, show window and return pointer
    w->setPrivate(true);
    w->show();
    return w;
}

void BrowserApplication::updateBookmarkMenus()
{
    for (int i = 0; i < m_browserWindows.size(); ++i)
    {
        QPointer<MainWindow> m = m_browserWindows.at(i);
        if (!m.isNull())
            m->refreshBookmarkMenu();
    }
}

void BrowserApplication::clearHistory(HistoryType histType, QDateTime start)
{
    // Check if browsing history flag is set
    if ((histType & HistoryType::Browsing) == HistoryType::Browsing)
    {
        if (start.isNull())
            m_historyMgr->clearAllHistory();
        else
            m_historyMgr->clearHistoryFrom(start);

        resetHistoryMenus();
    }

    // Check if cookie flag is set
    if ((histType & HistoryType::Cookies) == HistoryType::Cookies)
    {
        if (start.isNull())
            m_cookieJar->eraseAllCookies();
        else
            m_cookieJar->clearCookiesFrom(start);
    }

    //todo: support clearing form and search data
}

void BrowserApplication::resetHistoryMenus()
{
    for (int i = 0; i < m_browserWindows.size(); ++i)
    {
        QPointer<MainWindow> m = m_browserWindows.at(i);
        if (!m.isNull())
        {
            // Remove current history and set items to updated content in history manager
            m->clearHistoryItems();
            setHistoryForWindow(m);
        }
    }
}

void BrowserApplication::resetUserAgentMenus()
{
    for (int i = 0; i < m_browserWindows.size(); ++i)
    {
        QPointer<MainWindow> m = m_browserWindows.at(i);
        if (!m.isNull())
            m->resetUserAgentMenu();
    }
}

void BrowserApplication::setHistoryForWindow(MainWindow *w)
{
    if (!w)
        return;

    // Add recent history to window
    const QList<WebHistoryItem> &historyItems = m_historyMgr->getRecentItems();
    for (auto it : historyItems)
    {
        if (!it.Title.isEmpty())
            w->addHistoryItem(it.URL, it.Title, m_faviconStorage->getFavicon(it.URL));
    }
}

void BrowserApplication::setWebSettings()
{
    // Check if custom user agent is used
    if (m_settings->getValue("CustomUserAgent").toBool())
        WebPage::setUserAgent(m_userAgentMgr->getUserAgent().Value);

    QDir iconPath(QDir::homePath() + QDir::separator() + ".cache" + QDir::separator() + "Vaccarelli");
    if (!iconPath.exists())
        iconPath.mkpath(iconPath.absolutePath());
    QWebSettings::setIconDatabasePath(iconPath.absolutePath());

    QWebSettings *settings = QWebSettings::globalSettings();
    settings->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
    settings->setAttribute(QWebSettings::JavascriptEnabled, m_settings->getValue("EnableJavascript").toBool());
    settings->setAttribute(QWebSettings::JavascriptCanOpenWindows, m_settings->getValue("EnableJavascriptPopups").toBool());
    settings->setAttribute(QWebSettings::AutoLoadImages, m_settings->getValue("AutoLoadImages").toBool());
    settings->setAttribute(QWebSettings::PluginsEnabled, m_settings->getValue("EnablePlugins").toBool());
    settings->setAttribute(QWebSettings::XSSAuditingEnabled, m_settings->getValue("EnableXSSAudit").toBool());
    settings->setAttribute(QWebSettings::MediaSourceEnabled, true);

    settings->setFontFamily(QWebSettings::StandardFont, m_settings->getValue("StandardFont").toString());
    settings->setFontSize(QWebSettings::DefaultFontSize, m_settings->getValue("StandardFontSize").toInt());
}
