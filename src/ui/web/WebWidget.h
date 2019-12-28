#ifndef WEBWIDGET_H
#define WEBWIDGET_H

#include "ServiceLocator.h"
#include "WebState.h"

#include <QIcon>
#include <QMetaType>
#include <QPointer>
#include <QUrl>
#include <QWidget>

#include <QtGlobal>
#include <QtWebEngineCoreVersion>

namespace adblock {
    class AdBlockManager;
}

class BrowserTabWidget;
class FaviconManager;
class HttpRequest;
class MainWindow;
class WebHistory;
class WebInspector;
class WebLoadObserver;
class WebPage;
class WebView;
class WebWidget;

class QHideEvent;
class QShowEvent;

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
     * @param serviceLocator Web browser service registry / locator
     * @param privateMode Set to true if the web view should be off-the-record, false if a regular web view
     * @param parent Pointer to the parent widget
     */
    explicit WebWidget(const ViperServiceLocator &serviceLocator, bool privateMode, QWidget *parent = nullptr);

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

    /// Loads the specified url and displays its contents in the web view.
    void load(const QUrl &url, bool wasEnteredByUser = false);

    /// Loads the given HTTP request onto the page
    void load(const HttpRequest &request);

    /// Returns a pointer to the page's history
    WebHistory *getHistory() const;

    /// Returns the widget's web history as a serialized byte array
    QByteArray getEncodedHistory() const;

    /// Returns true if the web inspector is connected to the current page, false if otherwise
    bool isInspectorActive() const;

    /// Returns a pointer to the web inspector, instantiating it if not already done so
    WebInspector *getInspector();

    /// Returns the current url
    QUrl url() const;

    /// Returns the URL associated with the page after a navigation request was accepted, but before it was fully loaded.
    QUrl getOriginalUrl() const;

    /// Returns a pointer to the web page
    WebPage *page() const;

    /// Returns a pointer to the web view
    WebView *view() const;

    /// Sets the state of the web widget as it was during a hibernation event
    void setWebState(WebState &&state);

    /// Returns the last url that was manually typed by the user, or an empty url if not applicable
    const QUrl &getLastTypedUrl() const;

    /// Clears the record of the last url that was manually typed by the user.
    void clearLastTypedUrl();

    /// Event filter
    bool eventFilter(QObject *watched, QEvent *event) override;

public Q_SLOTS:
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

Q_SIGNALS:
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

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    /// Handler for web widget hide events
    void hideEvent(QHideEvent *event) override;

    /// Handler for the web widget show event
    void showEvent(QShowEvent *event) override;

    /// Handler for the lifecycle state check event
    void timerEvent(QTimerEvent *event) override;
#endif

private Q_SLOTS:
    /// Shows the context menu on the web page
    void showContextMenuForView();

    /// Handles a tab pinned state change event notification
    void onTabPinned(int index, bool value);

private:
    /// Forces a repaint of the inner web page
    void forceRepaint();

    /// Saves the state of the web widget before it is hibernated
    void saveState();

    /// Instantiates the web view and its page, binding them to the web widget
    void setupWebView();

private:
    /// Web browser service locator
    const ViperServiceLocator &m_serviceLocator;

    /// Pointer to the advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    /// Identifier of the timer that checks if the webpage should be frozen
    int m_lifecycleFreezeTimerId;

    /// Identifier of the timer that checks if the webpage should be discarded
    int m_lifecycleDiscardTimerId;
#endif

    /// The web page being shown by the web widget, unless the page is hibernating
    WebPage *m_page;

    /// The actual web view being used to render and access web content
    WebView *m_view;

    /// The web page inspector associated with this widget
    WebInspector *m_inspector;

    /// Listens for the loadFinished signal if this web widget is not associated with a private browsing profile
    WebLoadObserver *m_pageLoadObserver;

    /// Pointer to the widget's parent window
    MainWindow *m_mainWindow;

    /// Pointer to the favicon manager
    FaviconManager *m_faviconManager;

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

    /// Last URL typed by the user
    QUrl m_lastTypedUrl;
};

#endif // WEBWIDGET_H
