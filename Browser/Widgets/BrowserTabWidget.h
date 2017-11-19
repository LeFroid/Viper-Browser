#ifndef BROWSERTABWIDGET_H
#define BROWSERTABWIDGET_H

#include "Settings.h"

#include <memory>
#include <QTabWidget>
#include <QUrl>

class BrowserTabBar;
class QMenu;
class WebView;

/**
 * @class BrowserTabWidget
 * @brief Handles rendering of browser tabs containing WebView widgets
 */
class BrowserTabWidget : public QTabWidget
{
    Q_OBJECT

    /// Which page to load by default when a new tab is created
    enum NewTabPage { HomePage = 0, BlankPage = 1 };

public:
    /// Constructs the browser tab widget with the given parent
    BrowserTabWidget(std::shared_ptr<Settings> settings, QWidget *parent = nullptr);

    /// Returns a pointer to the current web view
    WebView *currentWebView() const;

    /// Returns the web view at the given tab index, or a nullptr if tabIndex is invalid
    WebView *getWebView(int tabIndex) const;

    /// Sets private mode on if boolean passed is true, otherwise disables private mode
    void setPrivateMode(bool value);

signals:
    /// Emitted when a new tab is created, passing along a pointer to the associated web view
    void newTabCreated(WebView *view);

    /// Emitted when the current view has made progress loading its page. The range of value is [0,100]
    void loadProgress(int value);

    /// Emitted when the active tab / web view has changed
    void viewChanged(int index);

public slots:
    /// Called when a tab is to be closed
    void closeTab(int index = -1);

    /// Creates a new tab, returning its pointer. If makeCurrent is set to true, the new tab will gain
    /// focus as the active tab. If skipHomePage is set to true, the new tab will not automatically load the
    /// home page
    WebView *newTab(bool makeCurrent = false, bool skipHomePage = false);

    /// Called when the icon for a web view has changed
    void onIconChanged();

    /// Spawns a new browser tab, loading the given URL
    void openLinkInNewTab(const QUrl &url);

    /// Opens the given link in a new browser window, and sets the window to private mode if requested
    void openLinkInNewWindow(const QUrl &url, bool privateWindow);

    /// Loads the given URL in the active tab
    void loadUrl(const QUrl &url);

    /// Sets the back button and forward button history menus when a tab is changed
    void setNavHistoryMenus(QMenu *backMenu, QMenu *forwardMenu);

private slots:
    /// Called when the current tab has been changed
    void onCurrentChanged(int index);

    /// Resets the items in the back and forward button menus, populating them with the current tab's history
    void resetHistoryButtonMenus(bool ok);

private:
    /// Browser settings
    std::shared_ptr<Settings> m_settings;

    /// Private browsing flag
    bool m_privateBrowsing;

    /// Type of page to load for new tabs
    NewTabPage m_newTabPage;

    /// Active web view
    WebView *m_activeView;

    /// Custom tab bar
    BrowserTabBar *m_tabBar;

    /// Back button menu, from the \ref MainWindow toolbar
    QMenu *m_backMenu;

    /// Forward button menu, from the \ref MainWindow toolbar
    QMenu *m_forwardMenu;
};

#endif // BROWSERTABWIDGET_H
