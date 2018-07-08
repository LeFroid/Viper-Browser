#ifndef BOOKMARKBAR_H
#define BOOKMARKBAR_H

#include "BookmarkManager.h"
#include <QToolBar>
#include <QWidget>

class FaviconStorage;
class QHBoxLayout;
class QMenu;

/**
 * @class BookmarkBar
 * @brief Shown in the \ref MainWindow if the bookmark bar setting is enabled
 */
class BookmarkBar : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the bookmark bar widget
    explicit BookmarkBar(QWidget *parent = nullptr);

    /// Sets the pointer to the bookmark manager
    void setBookmarkManager(BookmarkManager *manager);

    /// Sets the pointer to the favicon store, used to load bookmark icons when refreshing the widget
    void setFaviconStore(FaviconStorage *faviconStore);

    /// Refreshes the items belonging to the bookmark bar
    void refresh();

signals:
    /// Emitted when the user requests to load a bookmark onto the active webview
    void loadBookmark(const QUrl &url);

    /// Emitted when the user requests to load a bookmark onto a new tab
    void loadBookmarkNewTab(const QUrl &url, bool makeCurrent = false);

    /// Emitted when the user requests to load a bookmark onto a new tab
    void loadBookmarkNewWindow(const QUrl &url);

private slots:
    /// Displays a context menu for a bookmark node at the given position
    void showBookmarkContextMenu(const QPoint &pos);

private:
    /// Adds items belonging to the given folder into the menu
    void addFolderItems(QMenu *menu, BookmarkNode *folder);

    /// Loads all bookmarks belonging to the given folder in new browser tabs
    void openFolderItemsNewTabs(BookmarkNode *folder);

    /**
     * @brief Loads all bookmarks belonging to the given folder in a new browser window
     * @param folder Pointer to the bookmark folder
     * @param privateWindow If true, the window will be set to private browsing mode. Defaults to false (public window)
     */
    void openFolderItemsNewWindow(BookmarkNode *folder, bool privateWindow = false);

    /// Clears any existing bookmarks from the widget
    void clear();

private:
    /// Pointer to the bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// Pointer to the favicon store
    FaviconStorage *m_faviconStore;

    /// Horizontal layout manager
    QHBoxLayout *m_layout;
};

#endif // BOOKMARKBAR_H
