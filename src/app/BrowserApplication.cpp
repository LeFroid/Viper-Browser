#include "BrowserApplication.h"
#include "BrowserScripts.h"
#include "AdBlockManager.h"
#include "AutoFill.h"
#include "BlockedSchemeHandler.h"
#include "BookmarkManager.h"
#include "BookmarkStore.h"
#include "CookieJar.h"
#include "CookieWidget.h"
#include "DatabaseFactory.h"
#include "DownloadManager.h"
#include "ExtStorage.h"
#include "FaviconStore.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"
#include "MainWindow.h"
#include "SearchEngineManager.h"
#include "Settings.h"
#include "NetworkAccessManager.h"
#include "RequestInterceptor.h"
#include "UserAgentManager.h"
#include "UserScriptManager.h"
#include "ViperSchemeHandler.h"
#include "WebPage.h"
#include "WebPageThumbnailStore.h"

#include <vector>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>

BrowserApplication::BrowserApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName(QLatin1String("Vaccarelli"));
    QCoreApplication::setApplicationName(QLatin1String("Viper Browser"));
    QCoreApplication::setApplicationVersion(QLatin1String("0.8"));

    setAttribute(Qt::AA_EnableHighDpiScaling, true);
    setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    setAttribute(Qt::AA_DontShowIconsInMenus, false);

    setWindowIcon(QIcon(QLatin1String(":/logo.png")));

    // Web profiles must be set up immediately upon browser initialization
    setupWebProfiles();

    // Instantiate and load settings
    m_settings = new Settings;
    if (!m_serviceLocator.addService(m_settings->objectName().toStdString(), m_settings))
        qWarning() << "Could not register application settings manager with service registry";

    // Initialize favicon storage module
    m_faviconStorage = DatabaseFactory::createWorker<FaviconStore>(m_settings->getPathValue(BrowserSetting::FaviconPath));
    if (!m_serviceLocator.addService(m_faviconStorage->objectName().toStdString(), m_faviconStorage.get()))
        qWarning() << "Could not register Favicon Store with service registry";

    // Initialize bookmarks store 
    m_bookmarkStore = DatabaseFactory::createWorker<BookmarkStore>(m_serviceLocator, m_settings->getPathValue(BrowserSetting::BookmarkPath));
    if (!m_serviceLocator.addService(m_bookmarkStore->objectName().toStdString(), m_bookmarkStore.get()))
        qWarning() << "Could not register Bookmark Store with service registry";
    if (!m_serviceLocator.addService(m_bookmarkStore->getNodeManager()->objectName().toStdString(), m_bookmarkStore->getNodeManager()))
        qWarning() << "Could not register Bookmark Manager with service registry";

    // Initialize cookie jar and cookie manager UI
    const bool enableCookies = m_settings->getValue(BrowserSetting::EnableCookies).toBool();
    m_cookieJar = new CookieJar(enableCookies, false);
    m_cookieJar->setThirdPartyCookiesEnabled(m_settings->getValue(BrowserSetting::EnableThirdPartyCookies).toBool());

    m_cookieUI = new CookieWidget();
    if (!m_serviceLocator.addService(m_cookieUI->objectName().toStdString(), m_cookieUI))
        qWarning() << "Could not register Bookmark Manager with service registry";

    // Get default profile and load cookies now that the cookie jar is instantiated
    auto webProfile = QWebEngineProfile::defaultProfile();
    webProfile->cookieStore()->loadAllCookies();

    // Initialize auto fill manager
    m_autoFill = new AutoFill(m_settings);
    if (!m_serviceLocator.addService(m_autoFill->objectName().toStdString(), m_autoFill))
        qWarning() << "Could not register AutoFill Manager with service registry";

    // Initialize download manager
    m_downloadMgr = new DownloadManager;
    m_downloadMgr->setDownloadDir(m_settings->getValue(BrowserSetting::DownloadDir).toString());
    if (!m_serviceLocator.addService(m_downloadMgr->objectName().toStdString(), m_downloadMgr))
        qWarning() << "Could not register Download Manager with service registry";

    connect(webProfile, &QWebEngineProfile::downloadRequested, m_downloadMgr, &DownloadManager::onDownloadRequest);
    connect(m_privateProfile, &QWebEngineProfile::downloadRequested, m_downloadMgr, &DownloadManager::onDownloadRequest);

    // Initialize advertisement blocking system
    m_adBlockManager = new AdBlockManager(m_serviceLocator, m_settings);
    if (!m_serviceLocator.addService("AdBlockManager", m_adBlockManager))
        qWarning() << "Could not register AdBlock Manager with service registry";

    // Instantiate the history manager and related systems
    m_historyMgr = DatabaseFactory::createWorker<HistoryManager>(m_serviceLocator, m_settings->getPathValue(BrowserSetting::HistoryPath));
    if (!m_serviceLocator.addService(m_historyMgr->objectName().toStdString(), m_historyMgr.get()))
        qWarning() << "Could not register History Manager with service registry";

    m_thumbnailStore = DatabaseFactory::createWorker<WebPageThumbnailStore>(m_serviceLocator, m_settings->getPathValue(BrowserSetting::ThumbnailPath));
    if (!m_serviceLocator.addService(m_thumbnailStore->objectName().toStdString(), m_thumbnailStore.get()))
            qWarning() << "Could not register Thumbnail Store with service registry";

    m_favoritePagesMgr = new FavoritePagesManager(m_historyMgr.get(), m_thumbnailStore.get(), m_settings->getPathValue(BrowserSetting::FavoritePagesFile));
    if (!m_serviceLocator.addService(m_favoritePagesMgr->objectName().toStdString(), m_favoritePagesMgr))
        qWarning() << "Could not register Favorite Web Page Manager with service registry";

    // Create network access manager
    m_networkAccessMgr = new NetworkAccessManager;
    m_networkAccessMgr->setCookieJar(m_cookieJar);

    m_downloadMgr->setNetworkAccessManager(m_networkAccessMgr);

    // Setup user agent manager before settings
    m_userAgentMgr = new UserAgentManager(m_settings);

    // Setup user script manager
    m_userScriptMgr = new UserScriptManager(m_settings);
    if (!m_serviceLocator.addService(m_userScriptMgr->objectName().toStdString(), m_userScriptMgr))
        qWarning() << "Could not register User Script Manager with service registry";

    // Setup extension storage manager
    m_extStorage = DatabaseFactory::createWorker<ExtStorage>(m_settings->getPathValue(BrowserSetting::ExtensionStoragePath));
    if (!m_serviceLocator.addService(m_extStorage->objectName().toStdString(), m_extStorage.get()))
        qWarning() << "Could not register Web Extension Storage Manager with service registry";

    // Apply global web scripts
    installGlobalWebScripts();

    // Apply web settings
    m_settings->applyWebSettings();

    // Load search engine information
    SearchEngineManager::instance().loadSearchEngines(m_settings->getPathValue(BrowserSetting::SearchEnginesFile));

    // Load ad block subscriptions (will do nothing if disabled)
    m_adBlockManager->loadSubscriptions();

    // Set browser's saved sessions file
    m_sessionMgr.setSessionFile(m_settings->getPathValue(BrowserSetting::SessionFile));

    // Connect aboutToQuit signal to browser's session management slot
    connect(this, &BrowserApplication::aboutToQuit, this, &BrowserApplication::beforeBrowserQuit);

    // Set URL handlers while application is active
    const std::vector<QString> urlSchemes { QLatin1String("http"), QLatin1String("https"), QLatin1String("viper") };
    for (const QString &scheme : urlSchemes)
        QDesktopServices::setUrlHandler(scheme, this, "openUrl");
}

BrowserApplication::~BrowserApplication()
{
    delete m_downloadMgr;
    delete m_networkAccessMgr;
    delete m_userAgentMgr;
    delete m_userScriptMgr;
    delete m_privateProfile;
    delete m_requestInterceptor;
    delete m_viperSchemeHandler;
    delete m_blockedSchemeHandler;
    delete m_cookieUI;
    delete m_autoFill;
    delete m_favoritePagesMgr;
    delete m_adBlockManager;
    delete m_settings;
}

BrowserApplication *BrowserApplication::instance()
{
    return static_cast<BrowserApplication*>(QCoreApplication::instance());
}

AutoFill *BrowserApplication::getAutoFill()
{
    return m_autoFill;
}

BookmarkManager *BrowserApplication::getBookmarkManager()
{
    return m_bookmarkStore->getNodeManager();
}

CookieJar *BrowserApplication::getCookieJar()
{
    return m_cookieJar;
}

Settings *BrowserApplication::getSettings()
{
    return m_settings;
}

DownloadManager *BrowserApplication::getDownloadManager()
{
    return m_downloadMgr;
}

FaviconStore *BrowserApplication::getFaviconStore()
{
    return m_faviconStorage.get();
}

HistoryManager *BrowserApplication::getHistoryManager()
{
    return m_historyMgr.get();
}

NetworkAccessManager *BrowserApplication::getNetworkAccessManager()
{
    return m_networkAccessMgr;
}

QWebEngineProfile *BrowserApplication::getPrivateBrowsingProfile()
{
    return m_privateProfile;
}

UserAgentManager *BrowserApplication::getUserAgentManager()
{
    return m_userAgentMgr;
}

UserScriptManager *BrowserApplication::getUserScriptManager()
{
    return m_userScriptMgr;
}

WebPageThumbnailStore *BrowserApplication::getWebPageThumbnailStore()
{
    return m_thumbnailStore.get();
}

MainWindow *BrowserApplication::getWindowById(WId windowId) const
{
    for (auto it = m_browserWindows.begin(); it != m_browserWindows.end(); ++it)
    {
        QPointer<MainWindow> winPtr = *it;
        if (!winPtr.isNull() && winPtr->winId() == windowId)
        {
            return winPtr.data();
        }
    }

    return nullptr;
}

MainWindow *BrowserApplication::getNewWindow()
{
    bool firstWindow = m_browserWindows.empty();

    MainWindow *w = new MainWindow(m_serviceLocator, false);
    m_browserWindows.append(w);
    connect(w, &MainWindow::aboutToClose, this, &BrowserApplication::maybeSaveSession);
    connect(w, &MainWindow::destroyed, [this, w](){
        if (m_browserWindows.contains(w))
            m_browserWindows.removeOne(w);
    });

    w->show();

    // Check if this is the first window since the application has started - if so, handle
    // the startup mode behavior depending on the user's configuration setting
    if (firstWindow)
    {
        StartupMode mode = static_cast<StartupMode>(m_settings->getValue(BrowserSetting::StartupMode).toInt());
        switch (mode)
        {
            case StartupMode::LoadHomePage:
                w->loadUrl(QUrl::fromUserInput(m_settings->getValue(BrowserSetting::HomePage).toString()));
                break;
            case StartupMode::LoadBlankPage:
                w->loadBlankPage();
                break;
            case StartupMode::LoadNewTabPage:
                w->loadUrl(QUrl(QLatin1String("viper://newtab")));
                break;
            case StartupMode::RestoreSession:
                m_sessionMgr.restoreSession(w);
                break;
        }

        m_adBlockManager->updateSubscriptions();
    }

    return w;
}

MainWindow *BrowserApplication::getNewPrivateWindow()
{
    MainWindow *w = new MainWindow(m_serviceLocator, true);
    m_browserWindows.append(w);
    connect(w, &MainWindow::destroyed, [this, w](){
        if (m_browserWindows.contains(w))
            m_browserWindows.removeOne(w);
    });

    w->show();
    return w;
}

void BrowserApplication::openUrl(const QUrl &url)
{
    if (MainWindow *win = qobject_cast<MainWindow*>(activeWindow()))
    {
        win->openLinkNewTab(url);
        return;
    }

    for (QPointer<MainWindow> &win : m_browserWindows)
    {
        if (!win.isNull())
        {
            win->openLinkNewTab(url);
            return;
        }
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

        emit resetHistoryMenu();
    }

    if ((histType & HistoryType::Cache) == HistoryType::Cache)
        QWebEngineProfile::defaultProfile()->clearHttpCache();

    //todo: support clearing form and search data
}

void BrowserApplication::clearHistoryRange(HistoryType histType, std::pair<QDateTime, QDateTime> range)
{
    // Check for a valid start-end date-time pair
    if (!range.first.isValid() || !range.second.isValid())
        return;

    // Check if browsing history flag is set
    if ((histType & HistoryType::Browsing) == HistoryType::Browsing)
    {
        m_historyMgr->clearHistoryInRange(range);
        emit resetHistoryMenu();
    }

    if ((histType & HistoryType::Cache) == HistoryType::Cache)
        QWebEngineProfile::defaultProfile()->clearHttpCache();

    //todo: support clearing form and search data
}

void BrowserApplication::installGlobalWebScripts()
{
    BrowserScripts browserScriptContainer;

    auto publicScriptCollection = QWebEngineProfile::defaultProfile()->scripts();
    auto privateScriptCollection = m_privateProfile->scripts();

    const std::vector<QWebEngineScript> &globalScripts = browserScriptContainer.getGlobalScripts();
    for (const QWebEngineScript &script : globalScripts)
    {
        publicScriptCollection->insert(script);
        privateScriptCollection->insert(script);
    }

    const std::vector<QWebEngineScript> &publicScripts = browserScriptContainer.getPublicOnlyScripts();
    for (const QWebEngineScript &script :publicScripts)
    {
        publicScriptCollection->insert(script);
    }
}

void BrowserApplication::setupWebProfiles()
{
    // Only two profiles for now, standard and private
    auto webProfile = QWebEngineProfile::defaultProfile();
    m_privateProfile = new QWebEngineProfile(this);

    // Instantiate request interceptor
    m_requestInterceptor = new RequestInterceptor(m_serviceLocator, this);

    // Instantiate scheme handlers
    m_viperSchemeHandler = new ViperSchemeHandler(this);
    m_blockedSchemeHandler = new BlockedSchemeHandler(m_serviceLocator, this);

    // Attach request interceptor and scheme handlers to web profiles
    webProfile->setRequestInterceptor(m_requestInterceptor);
    webProfile->installUrlSchemeHandler("viper", m_viperSchemeHandler);
    webProfile->installUrlSchemeHandler("blocked", m_blockedSchemeHandler);

    m_privateProfile->setRequestInterceptor(m_requestInterceptor);
    m_privateProfile->installUrlSchemeHandler("viper", m_viperSchemeHandler);
    m_privateProfile->installUrlSchemeHandler("blocked", m_blockedSchemeHandler);
}

void BrowserApplication::beforeBrowserQuit()
{
    const StartupMode mode = static_cast<StartupMode>(m_settings->getValue(BrowserSetting::StartupMode).toInt());

    // Get all windows that can be saved
    std::vector<MainWindow*> windows;
    for (int i = 0; i < m_browserWindows.size(); ++i)
    {
        QPointer<MainWindow> m = m_browserWindows.at(i);
        if (!m.isNull() && !m->isPrivate())
        {
            windows.push_back(m.data());
        }
    }

    if (!windows.empty() && mode == StartupMode::RestoreSession && !m_sessionMgr.alreadySaved())
        m_sessionMgr.saveState(windows);
}

void BrowserApplication::maybeSaveSession()
{
    // Note: don't need to check if startup mode is set to restore session, this slot will not be
    //       activated unless that condition is already met
    std::vector<MainWindow*> windows;
    for (int i = 0; i < m_browserWindows.size(); ++i)
    {
        QPointer<MainWindow> m = m_browserWindows.at(i);
        if (!m.isNull() && !m->isPrivate())
        {
            windows.push_back(m.data());
        }
    }

    // Only save session in this method if there's one window left. Saving more than one window is
    // handled by beforeBrowserQuit() method
    if (windows.empty() || windows.size() > 1)
        return;

    m_sessionMgr.saveState(windows);
}
