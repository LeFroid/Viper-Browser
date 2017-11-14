#ifndef URLLINEEDIT_H
#define URLLINEEDIT_H

#include <QLineEdit>
#include <QUrl>

class QToolButton;

/// Security icon types used by the line edit
enum class SecurityIcon
{
    /// HTTP
    Standard = 0,
    /// HTTPS
    Secure = 1,
    /// HTTPS with invalid certificate
    Insecure = 2
};

/**
 * @class URLLineEdit
 * @brief Used to enter an address to visit in the browser window. Includes suggestions based on history
 *        and displays whether or not the current WebPage is secure
 */
class URLLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    /// Constructs the URL line edit
    explicit URLLineEdit(QWidget *parent = nullptr);

    /// Sets the security icon at the left side of the line edit widget
    void setSecurityIcon(SecurityIcon iconType);

    /// Sets the URL to be displayed with a security status icon in the line edit widget
    void setURL(const QUrl &url);

signals:
    /// Called when the user requests to view the security information regarding the current page
    void viewSecurityInfo();

protected:
    /// Paints the line edit with an icon that shows whether or not the current site is secure
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    /// Button illustrating if website is secure, insecure or if certificate is expired or revoked
    QToolButton *m_securityButton;
};

#endif // URLLINEEDIT_H
