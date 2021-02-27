#ifndef URLLINEEDIT_H
#define URLLINEEDIT_H

#include "ServiceLocator.h"

#include <unordered_map>
#include <vector>

#include <QLineEdit>
#include <QString>
#include <QTextLayout>
#include <QUrl>

class BookmarkManager;
class BookmarkNode;
class MainWindow;
class URLSuggestionWidget;
class WebWidget;
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
    friend class NavigationToolBar;

    Q_OBJECT

public:
    /// Constructs the URL line edit
    explicit URLLineEdit(QWidget *parent = nullptr);

    /// URLLineEdit destructor
    ~URLLineEdit();

    /// Returns a pointer to the bookmark node of the page with the URL currently in the widget, or a
    /// null pointer if the current page is not bookmarked
    BookmarkNode *getBookmarkNode() const;

    /// Sets the bookmark state of the current page, to be reflected by the bookmark icon
    void setCurrentPageBookmarked(bool isBookmarked, BookmarkNode *node = nullptr);

    /// Sets the security icon at the left side of the line edit widget
    void setSecurityIcon(SecurityIcon iconType);

    /// Called when the browser window's tab has changed - stores any user text in a hash map so the contents
    /// will not be lost if they go back to the tab
    void tabChanged(WebWidget *newView);

Q_SIGNALS:
    /// Emitted when the user has requested to load the given url into the active WebWidget
    void loadRequested(const QUrl &url);

    /// Called when the user requests to view the security information regarding the current page
    void viewSecurityInfo();

    /// Emitted when the user wants to change the bookmark status of the page they are currently visiting
    void toggleBookmarkStatus();

public Q_SLOTS:
    /// Removes the given view pointer from the map of webviews to user-set text in the line edit
    void removeMappedView(WebWidget *view);

    /// Sets the URL to be displayed with a security status icon in the url bar
    void setURL(const QUrl &url);

private Q_SLOTS:
    /// Handles the returnPressed event - checks for a valid URL or shortcut to a URL, and emits the loadRequested(const QUrl &) signal
    void onInputEntered();

    /// Called when a URL from the \ref URLSuggestionWidget is chosen by the user
    void onSuggestedURLChosen(const QUrl &url);

    /// Called whenever the text within the line edit is edited by the user
    void onTextEdited(const QString &text);

    /// Checks if the user input can be formatted as a URL.
    void tryToFormatInput(const QString &text);

protected:
    /// Paints the line edit with an icon that shows whether or not the current site is secure
    virtual void resizeEvent(QResizeEvent *event) override;

    /// Sets the format of the text in the line edit
    void setTextFormat(const std::vector<QTextLayout::FormatRange> &formats);

    /// Passes the service locator on to the url widget. This widget does not require the service locator,
    /// but instead passes it on to the URL suggestion worker
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

private:
    /// Called in the onInputEntered() method, this checks the \ref BookmarkManager 's bookmark collection
    /// to see if the user input matches a shortcut or shortcut pattern for any bookmarks.
    /// If there is a match, this emits loadRequested(shortcutUrl) and returns true
    bool isInputForBookmarkShortcut(const QString &input);

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
    std::unordered_map<WebWidget*, QString> m_userTextMap;

    /// Pointer to the active web view
    WebWidget *m_activeWebView;

    /// URL suggestion widget
    URLSuggestionWidget *m_suggestionWidget;

    /// Points to the bookmark manager. Used to check for bookmark shortcuts and url patterns when a user enters some text
    /// in this widget
    BookmarkManager *m_bookmarkManager;

    /// Pointer to the bookmark node of the page associated with the current URL, or a null pointer if the page is not bookmarked
    BookmarkNode *m_bookmarkNode;

    /// Flag indicating which themed icons to use (dark or light)
    bool m_isDarkTheme;
};

#endif // URLLINEEDIT_H
