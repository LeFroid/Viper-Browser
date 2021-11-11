#include "BrowserApplication.h"
#include "BrowserScripts.h"
#include "AdBlockManager.h"
#include "AutoFill.h"
#include "BlockedSchemeHandler.h"
#include "BookmarkManager.h"
#include "BookmarkStore.h"
#include "BrowserIPC.h"
#include "CommonUtil.h"
#include "CookieJar.h"
#include "CookieWidget.h"
#include "DatabaseFactory.h"
#include "DownloadManager.h"
#include "ExtStorage.h"
#include "FaviconManager.h"
#include "FaviconStore.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"
#include "HistoryStore.h"
#include "MainWindow.h"
#include "SecurityManager.h"
#include "SearchEngineManager.h"
#include "Settings.h"
#include "NetworkAccessManager.h"
#include "RequestInterceptor.h"
#include "UserAgentManager.h"
#include "UserScriptManager.h"
#include "ViperSchemeHandler.h"
#include "WebPage.h"
#include "WebPageThumbnailStore.h"
#include "WebSettings.h"
#include "WebWidget.h"
#include "config.h"

#include <cmath>
#include <vector>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QPluginLoader>
#include <QUrl>
#include <QDebug>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

BrowserApplication::BrowserApplication(BrowserIPC *ipc, int &argc, char **argv) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName(QLatin1String("Vaccarelli"));
    QCoreApplication::setApplicationName(QLatin1String("Viper-Browser"));
    QCoreApplication::setApplicationVersion(QLatin1String(VIPER_VERSION_STR));

    setAttribute(Qt::AA_DontShowIconsInMenus, false);

    setWindowIcon(QIcon(QLatin1String(":/logo.png")));

    // Web profiles must be set up immediately upon browser initialization
    setupWebProfiles();

    // Set pointer to the IPC handler. This is checked for pending messages on a regular basis
    m_ipc = ipc;

    // Check for IPC messages every 5 seconds
    m_ipcTimerId = startTimer(1000 * 5);

    // Instantiate and load settings
    m_settings = new Settings(m_defaultProfile->settings());
    registerService(m_settings);

    // Initialize favicon storage module
    //m_databaseScheduler.addWorker("FaviconStore",
    //                              std::bind(DatabaseFactory::createDBWorker<FaviconStore>, m_settings->getPathValue(BrowserSetting::FaviconPath)));
    m_faviconMgr = new FaviconManager(m_settings->getPathValue(BrowserSetting::FaviconPath));
    registerService(m_faviconMgr);

    // Bookmark setup
    m_databaseScheduler.addWorker("BookmarkStore",
                                  std::bind(DatabaseFactory::createDBWorker<BookmarkStore>, m_settings->getPathValue(BrowserSetting::BookmarkPath)));
    m_bookmarkManager = new BookmarkManager(m_serviceLocator, m_databaseScheduler, nullptr);
    registerService(m_bookmarkManager);

    // Initialize cookie jar and cookie manager UI
    m_cookieJar = new CookieJar(m_settings, m_defaultProfile, false);
    registerService(m_cookieJar);

    m_cookieUI = new CookieWidget(m_defaultProfile);
    registerService(m_cookieUI);

    // Get default profile and load cookies now that the cookie jar is instantiated
    m_defaultProfile->cookieStore()->loadAllCookies();

    // Initialize auto fill manager
    m_autoFill = new AutoFill(m_settings);
    registerService(m_autoFill);

    // Initialize download manager
    const std::vector<QWebEngineProfile*> webProfiles { m_defaultProfile, m_privateProfile };
    m_downloadMgr = new DownloadManager(m_settings, webProfiles);
    registerService(m_downloadMgr);

    // Initialize advertisement blocking system
    m_adBlockManager = new adblock::AdBlockManager(m_serviceLocator, m_settings);
    registerService(m_adBlockManager);

    // Instantiate the history manager and related systems
    m_databaseScheduler.addWorker("HistoryStore",
                                  std::bind(DatabaseFactory::createDBWorker<HistoryStore>, m_settings->getPathValue(BrowserSetting::HistoryPath)));
    m_historyMgr = new HistoryManager(m_serviceLocator, m_databaseScheduler);
    registerService(m_historyMgr);

    m_thumbnailStore = DatabaseFactory::createWorker<WebPageThumbnailStore>(m_serviceLocator, m_settings->getPathValue(BrowserSetting::ThumbnailPath));
    registerService(m_thumbnailStore.get());

    m_favoritePagesMgr = new FavoritePagesManager(m_historyMgr, m_thumbnailStore.get(), m_settings->getPathValue(BrowserSetting::FavoritePagesFile));
    registerService(m_favoritePagesMgr);

    // Create network access manager
    m_networkAccessMgr = new NetworkAccessManager;
    m_networkAccessMgr->setCookieJar(m_cookieJar);
    registerService(m_networkAccessMgr);

    m_downloadMgr->setNetworkAccessManager(m_networkAccessMgr);
    m_faviconMgr->setNetworkAccessManager(m_networkAccessMgr);

    // Setup user agent manager before settings
    m_userAgentMgr = new UserAgentManager(m_settings, m_defaultProfile);
    registerService(m_userAgentMgr);

    // Setup user script manager
    m_userScriptMgr = new UserScriptManager(m_downloadMgr, m_settings);
    registerService(m_userScriptMgr);

    // Setup extension storage manager
    m_extStorage = DatabaseFactory::createWorker<ExtStorage>(m_settings->getPathValue(BrowserSetting::ExtensionStoragePath));
    registerService(m_extStorage.get());

    // Apply global web scripts
    installGlobalWebScripts();

    // Apply web settings
    m_webSettings = new WebSettings(m_serviceLocator, m_defaultProfile->settings(), m_defaultProfile, m_privateProfile);

    // Load search engine information
    SearchEngineManager::instance().loadSearchEngines(m_settings->getPathValue(BrowserSetting::SearchEnginesFile));

    // Load ad block subscriptions (will do nothing if disabled)
    m_adBlockManager->loadSubscriptions();

    // Set browser's saved sessions file
    m_sessionMgr.setSessionFile(m_settings->getPathValue(BrowserSetting::SessionFile));

    // Inject services into the security manager
    SecurityManager::instance().setServiceLocator(m_serviceLocator);

    // Connect aboutToQuit signal to browser's session management slot
    // connect(this, &BrowserApplication::aboutToQuit, this, &BrowserApplication::beforeBrowserQuit);

    // Set URL handlers while application is active
    const std::vector<QString> urlSchemes { QLatin1String("http"), QLatin1String("https"), QLatin1String("viper") };
    for (const QString &scheme : urlSchemes)
        QDesktopServices::setUrlHandler(scheme, this, "openUrl");

    m_databaseScheduler.run();
}

BrowserApplication::~BrowserApplication()
{
    m_ipc = nullptr;
    killTimer(m_ipcTimerId);

    m_databaseScheduler.stop();

    delete m_downloadMgr;
    delete m_networkAccessMgr;
    delete m_userAgentMgr;
    delete m_userScriptMgr;
    delete m_privateProfile;
    delete m_viperSchemeHandler;
    delete m_blockedSchemeHandler;
    delete m_cookieUI;
    delete m_autoFill;
    delete m_favoritePagesMgr;
    delete m_historyMgr;
    delete m_adBlockManager;
    delete m_webSettings;
    delete m_faviconMgr;
    delete m_bookmarkManager;
    delete m_settings;
    delete m_defaultProfile;
}

BrowserApplication *BrowserApplication::instance()
{
    return static_cast<BrowserApplication*>(QCoreApplication::instance());
}

adblock::AdBlockManager *BrowserApplication::getAdBlockManager()
{
    return m_adBlockManager;
}

AutoFill *BrowserApplication::getAutoFill()
{
    return m_autoFill;
}

Settings *BrowserApplication::getSettings()
{
    return m_settings;
}

DownloadManager *BrowserApplication::getDownloadManager()
{
    return m_downloadMgr;
}

FaviconManager *BrowserApplication::getFaviconManager()
{
    return m_faviconMgr;
}

NetworkAccessManager *BrowserApplication::getNetworkAccessManager()
{
    return m_networkAccessMgr;
}

QWebEngineProfile *BrowserApplication::getPrivateBrowsingProfile()
{
    return m_privateProfile;
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

QObject *BrowserApplication::getService(const QString &serviceName) const
{
    return m_serviceLocator.getService(serviceName.toStdString());
}

bool BrowserApplication::isDarkTheme() const
{
    // Source: https://stackoverflow.com/questions/22603510/is-this-possible-to-detect-a-colour-is-a-light-or-dark-colour
    const QColor textColor = palette().text().color();
    const float hsp = std::sqrt(0.299 * (textColor.red() * textColor.red())
                                + 0.587 * (textColor.green() * textColor.green())
                                + 0.114 * (textColor.blue() * textColor.blue()));
    // Light text color, dark theme (and vice versa)
    return hsp > 127.5;
}

void BrowserApplication::prepareToQuit()
{
    beforeBrowserQuit();
    QCoreApplication::exit(0);
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
        loadPlugins();

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
                w->loadUrl(QUrl(QStringLiteral("viper://newtab")));
                break;
            case StartupMode::RestoreSession:
                m_sessionMgr.restoreSession(w, this);
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
    }

    if ((histType & HistoryType::Cache) == HistoryType::Cache)
        m_defaultProfile->clearHttpCache();

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
    }

    if ((histType & HistoryType::Cache) == HistoryType::Cache)
        m_defaultProfile->clearHttpCache();

    //todo: support clearing form and search data
}

void BrowserApplication::timerEvent(QTimerEvent *event)
{
    QApplication::timerEvent(event);

    if (event->timerId() == m_ipcTimerId)
    {
        checkBrowserIPC();
    }
}

void BrowserApplication::installGlobalWebScripts()
{
    BrowserScripts browserScriptContainer;

    auto publicScriptCollection = m_defaultProfile->scripts();
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

void BrowserApplication::registerService(QObject *service)
{
    if (!m_serviceLocator.addService(service->objectName().toStdString(), service))
        qWarning() << "Could not register " << service->objectName() << " with service registry";
}

void BrowserApplication::setupWebProfiles()
{
    // Only two profiles for now, standard and private
    m_defaultProfile = new QWebEngineProfile(QStringLiteral("Default"), this);
    m_defaultProfile->setObjectName(QLatin1String("PublicWebProfile"));
    registerService(m_defaultProfile);

    m_privateProfile = QWebEngineProfile::defaultProfile();
    m_privateProfile->setObjectName(QLatin1String("PrivateWebProfile"));
    registerService(m_privateProfile);

    // Instantiate scheme handlers
    m_viperSchemeHandler = new ViperSchemeHandler(this);
    m_blockedSchemeHandler = new BlockedSchemeHandler(m_serviceLocator, this);

    // Attach request interceptor and scheme handlers to web profiles
    m_defaultProfile->installUrlSchemeHandler("viper", m_viperSchemeHandler);
    m_defaultProfile->installUrlSchemeHandler("blocked", m_blockedSchemeHandler);

    m_privateProfile->installUrlSchemeHandler("viper", m_viperSchemeHandler);
    m_privateProfile->installUrlSchemeHandler("blocked", m_blockedSchemeHandler);
}

void BrowserApplication::checkBrowserIPC()
{
    if (m_ipc == nullptr || !m_ipc->hasMessage())
        return;

    std::vector<char> message = m_ipc->getMessage();
    if (message.empty() || message.at(0) == '\0')
        return;

    // Handle message
    std::string messageStdString(message.data(), message.size());
    QString messageStr = QString::fromStdString(messageStdString);

    if (messageStr.compare(QStringLiteral("new-window")) == 0)
    {
        static_cast<void>(getNewWindow());
    }
    else
    {
        // Try to first get the active window. If we can't get this, we will create a new window
        MainWindow *activeWin = qobject_cast<MainWindow*>(activeWindow());
        if (!activeWin)
        {
            for (QPointer<MainWindow> &win : m_browserWindows)
            {
                if (!win.isNull() && win->hasFocus())
                {
                    activeWin = win.data();
                    break;
                }
            }
        }

        if (!activeWin)
            activeWin = getNewWindow();

        QStringList urlList = messageStr.split(QChar('\t'), QStringSplitFlag::SkipEmptyParts);
        for (const QString &urlStr : urlList)
        {
            QUrl url = QUrl::fromUserInput(urlStr);
            if (!url.isEmpty() && !url.scheme().isEmpty() && url.isValid())
            {
                if (activeWin->currentWebWidget()->isOnBlankPage())
                    activeWin->loadUrl(url);
                else
                    activeWin->openLinkNewTab(url);
            }
        }
    }
}

void BrowserApplication::loadPlugins()
{
    QDir dir = QDir(VIPER_PLUGIN_DIR);
    const auto entryList = dir.entryList(QDir::Files);
    for (const QString &fileName : entryList)
    {
        QPluginLoader loader(dir.absoluteFilePath(fileName));
        if (QObject *plugin = loader.instance())
            registerService(plugin);
    }

    emit pluginsLoaded();
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
            m->prepareToClose();
        }
    }

    if (!windows.empty() && mode == StartupMode::RestoreSession && !m_sessionMgr.alreadySaved())
        m_sessionMgr.saveState(windows);

    for (QPointer<MainWindow> &win : m_browserWindows)
    {
        if (!win.isNull())
            win->close();
    }
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
