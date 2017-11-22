#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebView>

class WebLinkLabel;
class WebPage;
class QLabel;
class QMenu;

/**
 * @class WebView
 * @brief A widget that is used to view and edit web documents.
 */
class WebView : public QWebView
{
    Q_OBJECT

public:
    /// Constructs a web view
    explicit WebView(QWidget* parent = 0);

    /// Toggles private browsing mode
    void setPrivate(bool value);

    /// Returns the loading progress percentage value of the page as an integer in the range [0,100]
    int getProgress() const;

    /// Loads the resource of the blank page into the view
    void loadBlankPage();

public slots:
    /// Resets the zoom factor to its base value
    void resetZoom();

    /// Increases the zoom factor of the view by 10% of the base value
    void zoomIn();

    /// Decreases the zoom factor of the view by 10% of the base value
    void zoomOut();

private slots:
    /// Called when a download is requested
    void requestDownload(const QNetworkRequest &request);

protected:
    /// Event handler for context menu displays
    virtual void contextMenuEvent(QContextMenuEvent *event) override;

    /// Called to adjust the position of the link ref label
    virtual void resizeEvent(QResizeEvent *event) override;

    /// Called to hide the link ref label when the wheel is moved
    virtual void wheelEvent(QWheelEvent *event) override;

    /// Creates a new popup window on request
    virtual QWebView *createWindow(QWebPage::WebWindowType type) override;

signals:
    /// Called when the user requests to open a link in the current web view / tab
    void openRequest(const QUrl &url);

    /// Called when the user requests to open a link in a new browser tab
    void openInNewTabRequest(const QUrl &url);

    /// Called when the user requests to open a link in a new browser window
    void openInNewWindowRequest(const QUrl &url, bool privateWindow);

    /// Called when the user requests to inspect an element
    void inspectElement();

private:
    /// Adds a developer inspector option to the given menu, if the developer setting is enabled
    void addInspectorIfEnabled(QMenu *menu);

private:
    /// Label used to display the url of a link being hovered on by the user
    WebLinkLabel *m_labelLinkRef;

    /// Web page
    WebPage *m_page;

    /// Load progress
    int m_progress;
};

#endif // WEBVIEW_H
