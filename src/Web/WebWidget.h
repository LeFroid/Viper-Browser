#ifndef WEBWIDGET_H
#define WEBWIDGET_H

#include <QIcon>
#include <QMetaType>
#include <QPointer>
#include <QUrl>
#include <QWidget>

class BrowserTabWidget;
class HttpRequest;
class MainWindow;
class WebHistory;
class WebPage;
class WebView;
class WebWidget;

/// Contains the information about a \ref WebWidget that is needed to hibernate/wake a tab,
/// or save and restore a tab across browsing sessions
struct WebState
{
    /// The index of the page in its parent \ref BrowserTabWidget
    int index;

    /// True if the page is pinned in its \ref BrowserTabWidget, false if else
    bool isPinned;

    /// The icon associated with the page at its URL
    QIcon icon;

    /// The URL of the icon
    QUrl iconUrl;

    /// The title of the page its URL
    QString title;

    /// The page's current URL
    QUrl url;

    /// Serialized history of the page
    QByteArray pageHistory;

    /// Default constructor
    WebState() = default;

    /// Constructs the WebState, given a pointer to the WebWidget and its parent BrowserTabWidget
    WebState(WebWidget *webWidget, BrowserTabWidget *tabWidget);

    /// Serializes the WebState into a byte array
    QByteArray serialize() const;

    /// Deserializes the WebState from the given byte array
    void deserialize(QByteArray &data);
};

/**
 * @class WebWidget
 * @brief A widget that is used as the container of a \ref WebView and
 *        acts as a bridge between the browser and the web engine implementation
 */
class WebWidget : public QWidget
{
    friend class BrowserTabWidget;

    Q_OBJECT

public:
    /**
     * @brief Constructs the WebWidget
     * @param privateMode Set to true if the web view should be off-the-record, false if a regular web view
     * @param parent Pointer to the parent widget
     */
    explicit WebWidget(bool privateMode, QWidget *parent = nullptr);

    /// WebWidget destructor
    ~WebWidget();

    /// Returns the loading progress percentage value of the page as an integer in the range [0,100]
    int getProgress() const;

    /// Loads the resource of the blank page into the view
    void loadBlankPage();

    /// Returns true if the web widget is in hibernation mode, false if else
    bool isHibernating() const;

    /// Returns true if the view's page is blank, with no resources being loaded
    bool isOnBlankPage() const;

    /// Returns the icon associated with the current page
    QIcon getIcon() const;

    /// Returns the URL of the icon associated with the current page
    QUrl getIconUrl() const;

    /**
     * @brief Returns the title of the web page being viewed
     *
     * Returns the title of the web page being viewed. If there is no title
     * for the page, and the path of the URL is not empty or the root path,
     * the path following the last '/' will be returned as the title. If only
     * the root path is available, the host will be returned as the title
     * @return The title of the web page being viewed
     */
    QString getTitle() const;

    /// Returns the state of the web widget, used for serialization
    const WebState &getState();

    /// Loads the specified url and displays it
    void load(const QUrl &url);

    /// Loads the given HTTP request onto the page
    void load(const HttpRequest &request);

    /// Returns a pointer to the page's history
    WebHistory *getHistory() const;

    /// Returns the widget's web history as a serialized byte array
    QByteArray getEncodedHistory() const;

    /// Returns the current url
    QUrl url() const;

    /// Returns the URL associated with the page after a navigation request was accepted, but before it was fully loaded.
    QUrl getOriginalUrl() const;

    /// Returns a pointer to the web page
    WebPage *page() const;

    /// Returns a pointer to the web view
    WebView *view() const;

    /// Event filter
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    /// Reloads the current page
    void reload();

    /// Stops loading the current page
    void stop();

    /**
     * @brief Sets the hibernation status of the web widget.
     * @param on If true, the web widget's status will be saved and its WebView will be destroyed. If false, the
     *           web widget will reactivate its WebView
     */
    void setHibernation(bool on);

    /// Sets the state of the web widget as it was during a hibernation event
    void setWebState(WebState &state);

signals:
    /// Emitted when the widget is about to go into hibernation state
    void aboutToHibernate();

    /// Emitted when the widget is about to wake up from its hibernated state
    void aboutToWake();

    /// Emitted when the web view should be closed
    void closeRequest();

    /// Emitted when the icon associated with the view is changed. The new icon is specified by icon.
    void iconChanged(const QIcon &icon);

    /// Emitted when the URL of the icon associated with the view is changed. The new URL is specified by url.
    void iconUrlChanged(const QUrl &url);

    /// Emitted when the inspector should be shown
    void inspectElement();

    /// Emitted when a link is hovered over by the user
    void linkHovered(const QString &url);

    /// Emitted when the current page has finished loading. If parameter 'ok' is true, the load was successful.
    void loadFinished(bool ok);

    /// Emitted when the current page has loaded by the given percent, as an integer in range [0,100]
    void loadProgress(int value);

    /// Emitted when the user requests to open a link in the current web page
    void openRequest(const QUrl &url);

    /// Called when the user requests to open a link in a new browser tab
    void openInNewTab(const QUrl &url);

    /// Called when the user requests to open a link in a new browser tab without switching tabs
    void openInNewBackgroundTab(const QUrl &url);

    /// Emitted when the browser tab widget should create a new tab to load the given \ref HttpRequest
    void openHttpRequestInBackgroundTab(const HttpRequest &request);

    /// Called when the user requests to open a link in a new browser window
    void openInNewWindowRequest(const QUrl &url, bool privateWindow);

    /// Emitted when the title of the current page has changed to the given string
    void titleChanged(const QString &title);

    /// Emitted when the URL of the web page's main frame has changed
    void urlChanged(const QUrl &url);

protected:
    /// Handles mouse press events when the widget is in hibernate mode, otherwise forwards the event to the appropriate handler
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    /// Shows the context menu on the web page
    void showContextMenuForView();

private:
    /// Saves the state of the web widget before it is hibernated
    void saveState();

    /// Instantiates the web view and its page, binding them to the web widget
    void setupWebView();

private:
    /// The web page being shown by the web widget, unless the page is hibernating
    WebPage *m_page;

    /// The actual web view being used to render and access web content
    WebView *m_view;

    /// Pointer to the widget's parent window
    MainWindow *m_mainWindow;

    /// True if the widget's view is on a private browsing setting, false if else
    bool m_privateMode;

    /// Global and relative positions of the requested context menu from the active web view
    QPoint m_contextMenuPosGlobal, m_contextMenuPosRelative;

    /// Pointer to the WebView's focus proxy
    QPointer<QWidget> m_viewFocusProxy;

    /// True if the widget is in hibernation mode, false if else
    bool m_hibernating;

    /// Current state of the web page (icon, page title, url, etc.) in order to transition in and out of hibernation mode, close and re-open a tab, etc
    WebState m_savedState;
};

#endif // WEBWIDGET_H
