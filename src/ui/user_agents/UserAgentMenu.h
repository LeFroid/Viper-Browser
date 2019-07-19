#ifndef USERAGENTMENU_H
#define USERAGENTMENU_H

#include "ServiceLocator.h"
#include <QMenu>

class Settings;
class UserAgentManager;

class QActionGroup;

/**
 * @class UserAgentMenu
 * @brief Manages the browser's user agent control menu, found in the tools menu
 */
class UserAgentMenu : public QMenu
{
    friend class ToolMenu;

    Q_OBJECT

public:
    /// Constructs the user agent menu with the given parent
    UserAgentMenu(QWidget *parent = nullptr);

    /// Constructs the user agent menu with a title and a parent.
    UserAgentMenu(const QString &title, QWidget *parent = nullptr);

    /// Destroys the menu
    virtual ~UserAgentMenu();

protected:
    /// Passes a reference to the service locator, which allows the menu
    /// to fetch the instance of the User Agent manager as well as the
    /// application settings
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

public Q_SLOTS:
    /// Resets the items belonging to the user agent menu
    void resetItems();

private:
    /// Connects signals from the user agent manager to the appropriate UI update calls in the menu
    void setup();

private:
    /// Application settings
    Settings *m_settings;

    /// User agent manager
    UserAgentManager *m_userAgentManager;

    /// Action group for user agent items
    QActionGroup *m_userAgentGroup;
};

#endif // USERAGENTMENU_H
