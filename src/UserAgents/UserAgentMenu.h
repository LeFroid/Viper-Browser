#ifndef USERAGENTMENU_H
#define USERAGENTMENU_H

#include <QMenu>

class QActionGroup;

/**
 * @class UserAgentMenu
 * @brief Manages the browser's user agent control menu, found in the tools menu
 */
class UserAgentMenu : public QMenu
{
    Q_OBJECT

public:
    /// Constructs the user agent menu with the given parent
    UserAgentMenu(QWidget *parent = nullptr);

    /// Constructs the user agent menu with a title and a parent.
    UserAgentMenu(const QString &title, QWidget *parent = nullptr);

    /// Destroys the menu
    virtual ~UserAgentMenu();

public slots:
    /// Resets the items belonging to the user agent menu
    void resetItems();

private:
    /// Connects signals from the user agent manager to the appropriate UI update calls in the menu
    void setup();

private:
    /// Action group for user agent items
    QActionGroup *m_userAgentGroup;
};

#endif // USERAGENTMENU_H
