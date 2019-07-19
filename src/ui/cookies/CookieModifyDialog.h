#ifndef COOKIEMODIFYDIALOG_H
#define COOKIEMODIFYDIALOG_H

#include <QDialog>
#include <QNetworkCookie>

namespace Ui {
class CookieModifyDialog;
}

class QAbstractButton;

/**
 * @class CookieModifyDialog
 * @brief Allows the user to create or edit cookies
 */
class CookieModifyDialog : public QDialog
{
    Q_OBJECT

    /// Cookie expiration types
    enum class ExpireType { Date = 0, Session = 1 };

public:
    explicit CookieModifyDialog(QWidget *parent = 0);
    ~CookieModifyDialog();

    /// Sets the cookie to be used by the dialog
    void setCookie(const QNetworkCookie &cookie);

    /// Returns the current cookie's data
    const QNetworkCookie &getCookie() const;

private Q_SLOTS:
    /// Called when the cookie expiration type combo box value has changed
    void onExpireTypeChanged(int index);

    /// Saves the dialog input into the associated cookie, if the button activated was the 'save' button
    void setCookieData(QAbstractButton *button);

private:
    Ui::CookieModifyDialog *ui;

    /// Cookie associated with the current instance of the dialog
    QNetworkCookie m_cookie;
};

#endif // COOKIEMODIFYDIALOG_H
