#ifndef URLLINEEDIT_H
#define URLLINEEDIT_H

#include <QLineEdit>
#include <QUrl>

class MainWindow;
class QToolButton;

/// Security icon types used by the url line edit
enum class SecurityIcon
{
    /// HTTP
    Standard = 0,
    /// HTTPS
    Secure = 1,
    /// HTTPS with invalid certificate
    Insecure = 2
};

/// Bookmark icon types used by the url line edit
enum class BookmarkIcon
{
    /// Show no icon at all
    NoIcon = 0,
    /// Page is bookmarked
    Bookmarked = 1,
    /// Page is not bookmarked
    NotBookmarked = 2
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

    /// URLLineEdit destructor
    ~URLLineEdit();

    /// Sets the bookmark state of the current page, to be reflected by the bookmark icon
    void setCurrentPageBookmarked(bool isBookmarked);

    /// Sets the security icon at the left side of the line edit widget
    void setSecurityIcon(SecurityIcon iconType);

    /// Sets the URL to be displayed with a security status icon in the line edit widget
    void setURL(const QUrl &url);

    /// Returns a recommended size for the widget.
    QSize sizeHint() const override;

signals:
    /// Called when the user requests to view the security information regarding the current page
    void viewSecurityInfo();

    /// Emitted when the user wants to change the bookmark status of the page they are currently visiting
    void toggleBookmarkStatus();

protected:
    /// Paints the line edit with an icon that shows whether or not the current site is secure
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    /// Sets the bookmark icon at the right side of the line edit widget
    void setBookmarkIcon(BookmarkIcon iconType);

private:
    /// Button illustrating if website is secure, insecure or if certificate is expired or revoked
    QToolButton *m_securityButton;

    /// Button that indicates the bookmark status of the page and allows the user to add or remove the page from their bookmark collection
    QToolButton *m_bookmarkButton;

    /// Pointer to the browser window that created the line edit - used for convenience in sizeHint method
    MainWindow *m_parentWindow;
};

#endif // URLLINEEDIT_H
