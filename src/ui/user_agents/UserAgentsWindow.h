#ifndef USERAGENTSWINDOW_H
#define USERAGENTSWINDOW_H

#include <QWidget>

namespace Ui {
    class UserAgentsWindow;
}

class AddUserAgentDialog;
class UserAgentManager;
class QStandardItemModel;

/**
 * @class UserAgentsWindow
 * @brief User interface for the management of custom user agents
 */
class UserAgentsWindow : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the user agents window, given the pointer to the user agent manager and an optional parent
    explicit UserAgentsWindow(UserAgentManager *uaManager, QWidget *parent = 0);

    /// User agent window destructor
    ~UserAgentsWindow();

    /// Loads user agent information from the \ref UserAgentManager into the tree view, clearing any pre-existing tree items
    void loadUserAgents();

private Q_SLOTS:
    /// Spawns a dialog which allows the user to add a new category of user agents
    void addCategory();

    /// Spawns a dialog which allows the user to add a new user agent
    void addUserAgent();

    /// Called when the user clicks the "Ok" button from the \ref AddUserAgentDialog dialog, and adds the information to the model
    void onUserAgentAdded();

    /// Deletes the current selection in the tree, after confirming the user's choice with a dialog
    void deleteSelection();

    /// Called when the "Edit" button is clicked, allows the user to edit the user agent string of the current selection
    void editSelection();

    /// Called when an item in the tree view is clicked. Enables the edit and delete buttons if the index is valid
    void onTreeItemClicked(const QModelIndex &index);

    /// Saves any changes made to the custom user agents before closing the window
    void saveAndClose();

private:
    /// User interface elements
    Ui::UserAgentsWindow *ui;

    /// User agent manager
    UserAgentManager *m_userAgentManager;

    /// User agent tree model
    QStandardItemModel *m_model;

    /// Dialog used to add or modify user agents
    AddUserAgentDialog *m_addAgentDialog;
};

#endif // USERAGENTSWINDOW_H
