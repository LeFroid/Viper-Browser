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

#include <vector>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QDebug>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

BrowserApplication::BrowserApplication(int &argc, char **argv) :
    QApplication(argc, argv)
{
    QCoreApplication::setOrganizationName(QLatin1String("Vaccarelli"));
    QCoreApplication::setApplicationName(QLatin1String("Viper Browser"));
    QCoreApplication::setApplicationVersion(QLatin1String("0.9.1"));

    setAttribute(Qt::AA_EnableHighDpiScaling, true);
    setAttribute(Qt::AA_UseHighDpiPixmaps, true);
    setAttribute(Qt::AA_DontShowIconsInMenus, false);

    setWindowIcon(QIcon(QLatin1String(":/logo.png")));

    // Web profiles must be set up immediately upon browser initialization
    setupWebProfiles();

    // Instantiate and load settings
    m_settings = new Settings;
    registerService(m_settings);

    // Initialize favicon storage module
    //m_databaseScheduler.addWorker("FaviconStore",
    //                              std::bind(DatabaseFactory::createDBWorker<FaviconStore>, m_settings->getPathValue(BrowserSetting::FaviconPath)));
    m_faviconMgr = new FaviconManager(m_settings->getPathValue(BrowserSetting::FaviconPath));
    registerService(m_faviconMgr);
    //m_faviconStorage = DatabaseFactory::createWorker<FaviconStore>(m_settings->getPathValue(BrowserSetting::FaviconPath));
    //registerService(m_faviconStorage.get());

    // Bookmark setup
    m_databaseScheduler.addWorker("BookmarkStore",
                                  std::bind(DatabaseFactory::createDBWorker<BookmarkStore>, m_settings->getPathValue(BrowserSetting::BookmarkPath)));
    m_bookmarkManager = new BookmarkManager(m_serviceLocator, m_databaseScheduler, nullptr);
    registerService(m_bookmarkManager);

    // Initialize cookie jar and cookie manager UI
    const bool enableCookies = m_settings->getValue(BrowserSetting::EnableCookies).toBool();
    m_cookieJar = new CookieJar(enableCookies, false);
    m_cookieJar->setThirdPartyCookiesEnabled(m_settings->getValue(BrowserSetting::EnableThirdPartyCookies).toBool());
    registerService(m_cookieJar);

    m_cookieUI = new CookieWidget();
    registerService(m_cookieUI);

    // Get default profile and load cookies now that the cookie jar is instantiated
    auto webProfile = QWebEngineProfile::defaultProfile();
    webProfile->cookieStore()->loadAllCookies();

    // Initialize auto fill manager
    m_autoFill = new AutoFill(m_settings);
    registerService(m_autoFill);

    // Initialize download manager
    m_downloadMgr = new DownloadManager;
    m_downloadMgr->setDownloadDir(m_settings->getValue(BrowserSetting::DownloadDir).toString());
    registerService(m_downloadMgr);

    connect(webProfile, &QWebEngineProfile::downloadRequested, m_downloadMgr, &DownloadManager::onDownloadRequest);
    connect(m_privateProfile, &QWebEngineProfile::downloadRequested, m_downloadMgr, &DownloadManager::onDownloadRequest);

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
    m_userAgentMgr = new UserAgentManager(m_settings);
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
    m_webSettings = new WebSettings(m_serviceLocator, QWebEngineSettings::defaultSettings(), QWebEngineProfile::defaultProfile(), m_privateProfile);

    // Load search engine information
    SearchEngineManager::instance().loadSearchEngines(m_settings->getPathValue(BrowserSetting::SearchEnginesFile));

    // Load ad block subscriptions (will do nothing if disabled)
    m_adBlockManager->loadSubscriptions();

    // Set browser's saved sessions file
    m_sessionMgr.setSessionFile(m_settings->getPathValue(BrowserSetting::SessionFile));

    // Inject services into the security manager
    SecurityManager::instance().setServiceLocator(m_serviceLocator);

    // Connect aboutToQuit signal to browser's session management slot
    connect(this, &BrowserApplication::aboutToQuit, this, &BrowserApplication::beforeBrowserQuit);

    // Set URL handlers while application is active
    const std::vector<QString> urlSchemes { QLatin1String("http"), QLatin1String("https"), QLatin1String("viper") };
    for (const QString &scheme : urlSchemes)
        QDesktopServices::setUrlHandler(scheme, this, "openUrl");

    m_databaseScheduler.run();
}

BrowserApplication::~BrowserApplication()
{
    m_databaseScheduler.stop();

    delete m_downloadMgr;
    delete m_networkAccessMgr;
    delete m_userAgentMgr;
    delete m_userScriptMgr;
    delete m_privateProfile;
#if (QTWEBENGINECORE_VERSION < QT_VERSION_CHECK(5, 13, 0))
    delete m_requestInterceptor;
#endif
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
}

BrowserApplication *BrowserApplication::instance()
{
    return static_cast<BrowserApplication*>(QCoreApplication::instance());
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

void BrowserApplication::registerService(QObject *service)
{
    if (!m_serviceLocator.addService(service->objectName().toStdString(), service))
        qWarning() << "Could not register " << service->objectName() << " with service registry";
}

void BrowserApplication::setupWebProfiles()
{
    // Only two profiles for now, standard and private
    auto webProfile = QWebEngineProfile::defaultProfile();
    webProfile->setObjectName(QLatin1String("PublicWebProfile"));
    registerService(webProfile);

    m_privateProfile = new QWebEngineProfile(this);
    m_privateProfile->setObjectName(QLatin1String("PrivateWebProfile"));
    registerService(m_privateProfile);

#if (QTWEBENGINECORE_VERSION < QT_VERSION_CHECK(5, 13, 0))
    // Instantiate request interceptor
    m_requestInterceptor = new RequestInterceptor(m_serviceLocator, this);
    registerService(m_requestInterceptor);
#endif

    // Instantiate scheme handlers
    m_viperSchemeHandler = new ViperSchemeHandler(this);
    m_blockedSchemeHandler = new BlockedSchemeHandler(m_serviceLocator, this);

    // Attach request interceptor and scheme handlers to web profiles
#if (QTWEBENGINECORE_VERSION < QT_VERSION_CHECK(5, 13, 0))
    webProfile->setRequestInterceptor(m_requestInterceptor);
    m_privateProfile->setRequestInterceptor(m_requestInterceptor);
#endif

    webProfile->installUrlSchemeHandler("viper", m_viperSchemeHandler);
    webProfile->installUrlSchemeHandler("blocked", m_blockedSchemeHandler);

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
