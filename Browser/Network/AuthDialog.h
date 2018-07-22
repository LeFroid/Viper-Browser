#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QDialog>

namespace Ui {
    class AuthDialog;
}

/**
 * @class AuthDialog
 * @brief Dialog used to provide input to a QAuthenticator for standard or proxy authentication
 */
class AuthDialog : public QDialog
{
    friend class NetworkAccessManager;
    friend class WebPage;

    Q_OBJECT

public:
    /// Constructs the dialog with a given parent
    explicit AuthDialog(QWidget *parent = nullptr);

    /// AuthDialog destructor
    ~AuthDialog();

    /// Sets the message to be displayed above the credentials input region of the dialog
    void setMessage(const QString &message);

protected:
    /// Returns the username as entered by the user
    QString getUsername() const;

    /// Returns the password as entered by the user
    QString getPassword() const;

private:
    /// Pointer to the UI elements
    Ui::AuthDialog *ui;
};

#endif // AUTHDIALOG_H
