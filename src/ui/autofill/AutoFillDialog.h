#ifndef AUTOFILLDIALOG_H
#define AUTOFILLDIALOG_H

#include "CredentialStore.h"

#include <QDialog>

namespace Ui {
    class AutoFillDialog;
}

/**
 * @class AutoFillDialog
 * @brief Dialog that is shown when a form is submitted with new or
 *        updated credentials, allowing the user to decide what to do
 *        with them
 */
class AutoFillDialog : public QDialog
{
    Q_OBJECT

public:
    /// The types of events that trigger the dialog to be shown
    enum DialogAction
    {
        /// Newly entered credentials, dialog will ask to save or ignore them
        NewCredentialsAction    = 0,

        /// Updated password for existing credentials, dialog will ask to update or ignore
        UpdateCredentialsAction = 1
    };

public:
    /// Constructs the dialog with a given parent
    explicit AutoFillDialog(QWidget *parent = nullptr);

    /// Dialog destructor
    ~AutoFillDialog();

    /// Sets the UI elements to reflect the action being performed when the dialog is shown
    void setAction(DialogAction action);

    /// Sets the information needed before showing the dialog to the user
    void setInformation(const WebCredentials &credentials);

    /// Aligns the dialog towards the bottom left side of the \ref URLLineEdit on the \ref MainWindow that owns the
    /// web page that triggered the dialog to be shown
    void alignAndShow(const QRect &windowGeom);

Q_SIGNALS:
    /// Tells the \ref AutoFill manager to save the given credentials
    void saveCredentials(const WebCredentials &credentials);

    /// Tells the \ref AutoFill manager to update the given credentials
    void updateCredentials(const WebCredentials &credentials);

    /// Tells the \ref AutoFill manager to delete the given credentials
    void removeCredentials(const WebCredentials &credentials);

private Q_SLOTS:
    /// Called when the save credentials button is triggered
    void onSaveAction();

    /// Called when the update credentials button is triggered
    void onUpdateAction();

    /// Called when the remove credentials button is triggered
    void onRemoveAction();

private:
    /// Updates the internal \ref WebCredentials structure with the username and password in the dialog
    void updateWebCredentials();

private:
    /// Pointer to the UI elements
    Ui::AutoFillDialog *ui;

    /// Current action
    DialogAction m_actionType;

    /// The set of credentials being shown to the user
    WebCredentials m_credentials;
};

#endif // AUTOFILLDIALOG_H
