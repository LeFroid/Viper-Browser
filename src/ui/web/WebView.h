#ifndef WEBVIEW_H
#define WEBVIEW_H

#include "ServiceLocator.h"

#include <QPointer>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineView>

class HttpRequest;
class Settings;
class WebPage;

class QLabel;
class QMenu;

/**
 * @class WebView
 * @brief A widget that is used to view and edit web documents.
 */
class WebView : public QWebEngineView
{
    friend class MainWindow;
    friend class WebWidget;

    Q_OBJECT

public:
    /// Constructs a web view
    explicit WebView(bool privateView, QWidget* parent = nullptr);

    /// Destructor
    virtual ~WebView();

    /// Returns the loading progress percentage value of the page as an integer in the range [0,100]
    int getProgress() const;

    /// Loads the resource of the blank page into the view
    void loadBlankPage();

    /// Returns true if the view's page is blank, with no resources being loaded
    bool isOnBlankPage() const;

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

    /// Loads the specified url and displays it
    void load(const QUrl &url);

    /// Loads the given HTTP request
    void load(const HttpRequest &request);

    /// Returns a pointer to the renderer widget
    QWidget *getViewFocusProxy();

    /// Returns a pointer to the \ref WebPage
    WebPage *getPage() const;

    /// Returns a thumbnail of the current page, or a null QPixmap if the thumbnail could not be obtained
    const QPixmap &getThumbnail();

public Q_SLOTS:
    /// Resets the zoom factor to its base value
    void resetZoom();

    /// Increases the zoom factor of the view by 10% of the base value
    void zoomIn();

    /// Decreases the zoom factor of the view by 10% of the base value
    void zoomOut();

private Q_SLOTS:
    /// Called when the page has requested fullscreen mode
    void onFullScreenRequested(QWebEngineFullScreenRequest request);

    /// Called after a page is loaded successfully or not (status determined by 'ok')
    void onLoadFinished(bool ok);

protected:
    /// Initializes the view's \ref WebPage
    void setupPage(const ViperServiceLocator &serviceLocator);

    /// Sets the pointer to the render focus widget
    void setViewFocusProxy(QWidget *w);

protected:
    /// Event handler for context menu displays
    void showContextMenu(const QPoint &globalPos, const QPoint &relativePos);

    /// Does nothing
    void contextMenuEvent(QContextMenuEvent *event) override;

    /// Handles the mouse button release event, needed for the middle mouse button release event
    void _mouseReleaseEvent(QMouseEvent *event);

    /// Called to hide the link ref label when the wheel is moved
    void _wheelEvent(QWheelEvent *event);

    /// Handles drag events
    void dragEnterEvent(QDragEnterEvent *event) override;

    /// Handles drop events
    void dropEvent(QDropEvent *event) override;

    /// Handles resize events
    //void resizeEvent(QResizeEvent *event) override;

protected:
    /// returns the context menu helper script source, with the template parameters
    /// substituted for the coordinates given by parameter pos, scaled to the page's zoom factor
    QString getContextMenuScript(const QPoint &pos);

Q_SIGNALS:
    /// Called when the user requests to open a link in the current web view / tab
    void openRequest(const QUrl &url);

    /// Called when the user requests to open a link in a new browser tab
    void openInNewTab(const QUrl &url);

    /// Called when the user requests to open a link in a new browser tab without switching tabs
    void openInNewBackgroundTab(const QUrl &url);

    /// Emitted when the browser tab widget should create a new tab to load the given \ref HttpRequest
    void openHttpRequestInBackgroundTab(const HttpRequest &request);

    /// Called when the user requests to open a link in a new browser window
    void openInNewWindowRequest(const QUrl &url, bool privateWindow);

    /// Called when the user requests to inspect an element
    void inspectElement();

    /// Emitted when the page of the view has requested full screen to be enabled if on is true, or disabled if on is false
    void fullScreenRequested(bool on);

private:
    /// Generates a thumbnail of the current web page
    void makeThumbnailOfPage();

private:
    /// Web page behind this view
    WebPage *m_page;

    /// Application settings
    Settings *m_settings;

    /// Load progress
    int m_progress;

    /// True if the view is a private browsing view, false if else
    bool m_privateView;

    /// Context menu helper script template
    QString m_contextMenuHelper;

    /// Stores the result of an asynchronous javascript callback
    QVariant m_jsCallbackResult;

    /// Pointer to the WebView's focus proxy
    QPointer<QWidget> m_viewFocusProxy;

    /// Thumbnail of the current page
    QPixmap m_thumbnail;
};

#endif // WEBVIEW_H
