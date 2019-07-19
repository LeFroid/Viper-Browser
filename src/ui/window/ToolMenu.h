#ifndef TOOLMENU_H
#define TOOLMENU_H

#include "ServiceLocator.h"
#include <QMenu>

namespace adblock {
    class AdBlockManager;
}

class CookieWidget;
class DownloadManager;
class UserAgentMenu;
class UserScriptManager;

/**
 * @class ToolMenu
 * @brief Manages the tool menu, found in the \ref MainWindow menu bar
 */
class ToolMenu : public QMenu
{
    friend class MainWindow;

    Q_OBJECT

public:
    /// Constructs the tool menu with a given parent
    ToolMenu(QWidget *parent = nullptr);

    /// Constructs the tool menu with a title and a parent
    ToolMenu(const QString &title, QWidget *parent = nullptr);

    /// Destroys the menu
    ~ToolMenu();

protected:
    /// Passes a reference to the service locator, which allows the tool menu
    /// to bind its actions to the appropriate slots
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

public Q_SLOTS:
    /// Opens a window that allows the user to manage the advertisement blocking subscriptions and filters
    void openAdBlockManager();

    /// Opens the cookie management window
    void openCookieManager();

    /// Opens the user script management window
    void openUserScriptManager();

    /// Opens the download management window
    void openDownloadManager();

private:
    /// Initializes the actions belonging to the tool menu
    void setup();

// Dependencies
private:
    /// Points to the advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

    /// Points to the cookie jar management window
    CookieWidget *m_cookieWidget;

    /// Points to the user script manager
    UserScriptManager *m_userScriptManager;

    /// Points to the downloads window
    DownloadManager *m_downloadManager;

// Member variables
private:
    /// Advertisement blocking system management action (opens the \ref AdBlockWidget that allows user to add/remove/modify subscriptions)
    QAction *m_actionManageAdBlocker;

    /// Cookie management action (opens the \ref CookieWidget )
    QAction *m_actionManageCookies;

    /// User script management action (opens the \ref UserScriptWidget that allows user to add/remove/modify user scripts)
    QAction *m_actionManageUserScripts;

    /// User agent management menu
    UserAgentMenu *m_userAgentMenu;

    /// Downloads window opener action (see \ref DownloadManager )
    QAction *m_actionViewDownloads;
};

#endif // TOOLMENU_H
