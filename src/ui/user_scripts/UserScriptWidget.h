#ifndef USERSCRIPTWIDGET_H
#define USERSCRIPTWIDGET_H

#include <QWidget>

class UserScriptManager;

namespace Ui {
    class UserScriptWidget;
}

/**
 * @class UserScriptWidget
 * @brief Acts as a management window for user scripts. The user can install remote scripts by URL,
 *        create new scripts locally, modify existing scripts, delete scripts from storage, and enable
 *        or disable existing scripts.
 */
class UserScriptWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the management window, given a pointer to the user script manager
    explicit UserScriptWidget(UserScriptManager *userScriptManager);

    /// Destroys the UI elements associated with the object
    ~UserScriptWidget();

protected:
    /// Called to adjust the proportions of the columns belonging to the table views
    virtual void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    /// Called when an item in the table view is clicked. This enables the edit and delete buttons
    void onItemClicked(const QModelIndex &index);

    /// Called when the delete button is clicked. Will prompt the user for confirmation before deleting a user script
    void onDeleteButtonClicked();

    /// Called when the edit button is clicked. Will allow the user to edit the contents of the selected user script
    void onEditButtonClicked();

    /// Called when the install button is clicked. Spawns an input dialog and attempts to download and install the resource as a user script
    void onInstallButtonClicked();

    /// Called when the create button is clicked. Asks for some script meta-information from the user and creates a template they can edit
    void onCreateButtonClicked();

    /// Called after a user script has been created by the user and is ready to be loaded into a text editor
    void onScriptCreated(int scriptIdx);

private:
    /// Pointer to the user interface elements
    Ui::UserScriptWidget *ui;

    /// Points to the user script management system
    UserScriptManager *m_userScriptManager;
};

#endif // USERSCRIPTWIDGET_H
