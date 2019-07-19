#ifndef AUTOFILLCREDENTIALSVIEW_H
#define AUTOFILLCREDENTIALSVIEW_H

#include "CredentialStore.h"
#include <QWidget>

namespace Ui {
    class AutoFillCredentialsView;
}

class AutoFill;

/**
 * @class AutoFillCredentialsView
 * @brief Widget used to display the credentials that have been saved for
 *        the \ref AutoFill system
 */
class AutoFillCredentialsView : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the widget with a given parent
    explicit AutoFillCredentialsView(QWidget *parent = nullptr);

    /// Widget destructor
    ~AutoFillCredentialsView();

private Q_SLOTS:
    /// Called when one or more rows of data are to be removed from the \ref CredentialStore
    void onClickRemoveSelection();

    /// Called when the user clicks the "Remove all" credentials button
    void onClickRemoveAll();

private:
    /// Populates the table with the credentials saved for the AutoFill system
    void populateTable();

private:
    /// Pointer to the UI elements
    Ui::AutoFillCredentialsView *ui;

    /// Pointer to the AutoFill manager
    AutoFill *m_autoFill;

    /// Flag to track whether or not passwords are currently being shown in the table
    bool m_showingPasswords;

    /// The credentials shown in the table
    std::vector<WebCredentials> m_credentials;
};

#endif // AUTOFILLCREDENTIALSVIEW_H
