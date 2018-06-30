#ifndef URLLINEEDIT_H
#define URLLINEEDIT_H

#include <unordered_map>
#include <vector>
#include <QLineEdit>
#include <QString>
#include <QTextLayout>
#include <QUrl>

class MainWindow;
class URLSuggestionWidget;
class WebView;
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

    /// Called when the browser window's tab has changed - stores any user text in a hash map so the contents
    /// will not be lost if they go back to the tab
    void tabChanged(WebView *newView);

signals:
    /// Called when the user requests to view the security information regarding the current page
    void viewSecurityInfo();

    /// Emitted when the user wants to change the bookmark status of the page they are currently visiting
    void toggleBookmarkStatus();

public slots:
    /// Removes the given view pointer from the map of webviews to user-set text in the line edit
    void removeMappedView(WebView *view);

private slots:
    /// Called when a URL from the \ref URLSuggestionWidget is chosen by the user
    void onSuggestedURLChosen(const QUrl &url);

    /// Called whenever the text within the line edit is edited by the user
    void onTextEdited(const QString &text);

protected:
    /// Paints the line edit with an icon that shows whether or not the current site is secure
    virtual void resizeEvent(QResizeEvent *event) override;

    /// Sets the format of the text in the line edit
    void setTextFormat(const std::vector<QTextLayout::FormatRange> &formats);

private:
    /// Sets the bookmark icon at the right side of the line edit widget
    void setBookmarkIcon(BookmarkIcon iconType);

    /// Colors the given URL in the line edit based on its scheme, host, and path
    void setURLFormatted(const QUrl &url);

private:
    /// Button illustrating if website is secure, insecure or if certificate is expired or revoked
    QToolButton *m_securityButton;

    /// Button that indicates the bookmark status of the page and allows the user to add or remove the page from their bookmark collection
    QToolButton *m_bookmarkButton;

    /// Hashmap of WebView pointers and any text set by the user while the view was last active
    std::unordered_map<WebView*, QString> m_userTextMap;

    /// Pointer to the active web view
    WebView *m_activeWebView;

    /// URL suggestion widget
    URLSuggestionWidget *m_suggestionWidget;
};

#endif // URLLINEEDIT_H
