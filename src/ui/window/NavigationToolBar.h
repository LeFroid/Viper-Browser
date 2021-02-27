#ifndef NAVIGATIONTOOLBAR_H
#define NAVIGATIONTOOLBAR_H

#include "ServiceLocator.h"
#include <QToolBar>

class AdBlockButton;
class MainWindow;
class SearchEngineLineEdit;
class URLLineEdit;
class QSplitter;
class QToolButton;

/**
 * @class NavigationToolBar
 * @brief Manages the navigation bar and widgets belonging to the \ref MainWindow class.
 */
class NavigationToolBar : public QToolBar
{
    friend class MainWindow;

    Q_OBJECT

public:
    /// Constructs the toolbar with a given title and parent widget
    NavigationToolBar(const QString &title, QWidget *parent = nullptr);

    /// Constructs the toolbar with a given parent widget
    NavigationToolBar(QWidget *parent = nullptr);

    /// Navigation toolbar destructor
    ~NavigationToolBar();

    /// Returns a pointer to the search widget
    SearchEngineLineEdit *getSearchEngineWidget();

    /// Returns a pointer to the URL navigation widget
    URLLineEdit *getURLWidget();

    /// Called after the window has initialized the tab widget, connects elements between this widget and the tab widget
    void bindWithTabWidget();

    /// Sets the minimum heights for the toolbar and its sub-widgets
    void setMinHeights(int size);

Q_SIGNALS:
    /// Emitted when the ad block button is clicked
    void clickedAdBlockButton();

    /// Emitted when the user requests to manage their ad block filter subscriptions
    void requestManageAdBlockSubscriptions();

protected:
    /// Passes a reference to the browser service locator, used to fetch instances of the \ref AdBlockManager and \ref FaviconManager
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

private:
    /// Initializes the toolbar's properties and sub-widgets
    void setupUI();

    /// Returns a pointer to the toolbar's parent window
    MainWindow *getParentWindow();

private Q_SLOTS:
    /// Handles the tab change event that is sent by the \ref BrowserTabWidget
    void onTabChanged(int index);

    /// Called when the tab widget signals a page has progressed in loading its content
    void onLoadProgress(int value);

    /// Called when the stop/reload page action is triggered
    void onStopRefreshActionTriggered();

    /// Resets the items in the back and forward button menus, populating them with the current tab's history
    void onHistoryChanged();

private:
    /// Button to go to the previously visited page
    QToolButton *m_prevPage;

    /// Action to return to the next page after visiting a page through the previous page button
    QToolButton *m_nextPage;

    /// Action to either stop loading a page, or to refresh the page depending on its current state
    QAction *m_stopRefresh;

    /// URL navigation widget
    URLLineEdit *m_urlInput;

    /// Search engine line edit
    SearchEngineLineEdit *m_searchEngineLineEdit;

    /// Splitter that lets the user adjust the widths of the URL navigation widget and the search engine widget
    QSplitter *m_splitter;

    /// Button that is used to show information about advertisements being blocked on the current page
    AdBlockButton *m_adBlockButton;

    /// Flag indicating which themed icons to use (dark or light)
    bool m_isDarkTheme;
};

#endif // NAVIGATIONTOOLBAR_H
