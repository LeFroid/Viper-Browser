#include "AdBlockManager.h"
#include "AdBlockWidget.h"
#include "CookieWidget.h"
#include "DownloadManager.h"
#include "ToolMenu.h"
#include "UserAgentMenu.h"
#include "UserScriptManager.h"
#include "UserScriptWidget.h"

ToolMenu::ToolMenu(QWidget *parent) :
    QMenu(parent),
    m_adBlockManager(nullptr),
    m_cookieWidget(nullptr),
    m_userScriptManager(nullptr),
    m_downloadManager(nullptr),
    m_actionManageAdBlocker(nullptr),
    m_actionManageCookies(nullptr),
    m_actionManageUserScripts(nullptr),
    m_userAgentMenu(nullptr),
    m_actionViewDownloads(nullptr)
{
    setup();
}

ToolMenu::ToolMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent),
    m_adBlockManager(nullptr),
    m_cookieWidget(nullptr),
    m_userScriptManager(nullptr),
    m_downloadManager(nullptr),
    m_actionManageAdBlocker(nullptr),
    m_actionManageCookies(nullptr),
    m_actionManageUserScripts(nullptr),
    m_userAgentMenu(nullptr),
    m_actionViewDownloads(nullptr)
{
    setup();
}

ToolMenu::~ToolMenu()
{
}

void ToolMenu::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_adBlockManager    = serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager");
    m_cookieWidget      = serviceLocator.getServiceAs<CookieWidget>("CookieWidget");
    m_userScriptManager = serviceLocator.getServiceAs<UserScriptManager>("UserScriptManager");
    m_downloadManager   = serviceLocator.getServiceAs<DownloadManager>("DownloadManager");

    m_userAgentMenu->setServiceLocator(serviceLocator);
}

void ToolMenu::openAdBlockManager()
{
    if (!m_adBlockManager)
        return;

    AdBlockWidget *adBlockWidget = new AdBlockWidget(m_adBlockManager);
    adBlockWidget->show();
    adBlockWidget->raise();
    adBlockWidget->activateWindow();
}

void ToolMenu::openCookieManager()
{
    if (!m_cookieWidget)
        return;

    m_cookieWidget->resetUI();
    m_cookieWidget->show();
    m_cookieWidget->raise();
    m_cookieWidget->activateWindow();
}

void ToolMenu::openUserScriptManager()
{
    if (!m_userScriptManager)
        return;

    UserScriptWidget *userScriptWidget = new UserScriptWidget(m_userScriptManager);
    userScriptWidget->show();
    userScriptWidget->raise();
    userScriptWidget->activateWindow();
}

void ToolMenu::openDownloadManager()
{
    if (!m_downloadManager)
        return;

    m_downloadManager->show();
    m_downloadManager->raise();
    m_downloadManager->activateWindow();
}

void ToolMenu::setup()
{
    // Create menu items
    m_actionManageAdBlocker   = addAction(tr("Manage Ad Blocker"));
    m_actionManageCookies     = addAction(tr("Manage Cookies"));
    m_actionManageUserScripts = addAction(tr("Manage User Scripts"));

    m_userAgentMenu = new UserAgentMenu(tr("User Agents"), this);
    addMenu(m_userAgentMenu);

    addSeparator();

    m_actionViewDownloads = addAction(tr("View Downloads"));

    // Bind action triggered signals to their respective handlers
    connect(m_actionManageAdBlocker,   &QAction::triggered, this, &ToolMenu::openAdBlockManager);
    connect(m_actionManageCookies,     &QAction::triggered, this, &ToolMenu::openCookieManager);
    connect(m_actionManageUserScripts, &QAction::triggered, this, &ToolMenu::openUserScriptManager);
    connect(m_actionViewDownloads,     &QAction::triggered, this, &ToolMenu::openDownloadManager);
}
