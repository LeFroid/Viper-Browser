#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEngineContextMenuData>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineView>

class WebPage;
class QLabel;
class QMenu;

/**
 * @class WebView
 * @brief A widget that is used to view and edit web documents.
 */
class WebView : public QWebEngineView
{
    Q_OBJECT

public:
    /// Constructs a web view
    explicit WebView(bool privateView, QWidget* parent = 0);

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

public slots:
    /// Resets the zoom factor to its base value
    void resetZoom();

    /// Increases the zoom factor of the view by 10% of the base value
    void zoomIn();

    /// Decreases the zoom factor of the view by 10% of the base value
    void zoomOut();

    /// Displays the context menu at the given position
    void showContextMenu(const QPoint &globalPos, const QPoint &relativePos);

private slots:
    /// Called when the page has requested fullscreen mode
    void onFullScreenRequested(QWebEngineFullScreenRequest request);

protected:
    /// Event handler for context menu displays
    virtual void contextMenuEvent(QContextMenuEvent *event) override;

    /// Called to hide the link ref label when the wheel is moved
    virtual void wheelEvent(QWheelEvent *event) override;

    /// Creates a new popup window on request
    virtual QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

    /// Handles drag events
    virtual void dragEnterEvent(QDragEnterEvent *event) override;

    /// Handles drop events
    virtual void dropEvent(QDropEvent *event) override;

protected:
    /// returns the context menu helper script source, with the template parameters
    /// substituted for the coordinates given by parameter pos, scaled to the page's zoom factor
    QString getContextMenuScript(const QPoint &pos);

signals:
    /// Called when the user requests to open a link in the current web view / tab
    void openRequest(const QUrl &url);

    /// Called when the user requests to open a link in a new browser tab
    void openInNewTabRequest(const QUrl &url, bool makeCurrent = false);

    /// Called when the user requests to open a link in a new browser window
    void openInNewWindowRequest(const QUrl &url, bool privateWindow);

    /// Called when the user requests to inspect an element
    void inspectElement();

    /// Emitted when a link is hovered over by the user
    void linkHovered(const QUrl &url);

    /// Emitted when the view's page requests to be closed
    void viewCloseRequested();

    /// Emitted when the page of the view has requested full screen to be enabled if on is true, or disabled if on is false
    void fullScreenRequested(bool on);

private:
    /// Web page
    WebPage *m_page;

    /// Load progress
    int m_progress;

    /// True if the view is a private browsing view, false if else
    bool m_privateView;

    /// Context menu helper script template
    QString m_contextMenuHelper;

    /// Stores the result of an asynchronous javascript callback
    QVariant m_jsCallbackResult;
};

#endif // WEBVIEW_H
