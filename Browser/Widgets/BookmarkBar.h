#ifndef BOOKMARKBAR_H
#define BOOKMARKBAR_H

#include "BookmarkManager.h"
#include <memory>
#include <QToolBar>

class FaviconStorage;
class QMenu;

/**
 * @class BookmarkBar
 * @brief Shown in the \ref MainWindow if the bookmark bar setting is enabled
 */
class BookmarkBar : public QToolBar
{
    Q_OBJECT

public:
    /// Constructs the bookmark bar widget
    explicit BookmarkBar(QWidget *parent = nullptr);

    /// Sets the pointer to the bookmark manager
    void setBookmarkManager(std::shared_ptr<BookmarkManager> manager);

    /// Refreshes the items belonging to the bookmark bar
    void refresh();

signals:
    /// Emitted when the user requests to load a bookmark onto the active webview
    void loadBookmark(const QUrl &url);

//public slots:
    //

private:
    /// Adds items belonging to the given folder into the menu
    void addFolderItems(QMenu *menu, BookmarkFolder *folder, FaviconStorage *iconStorage);

private:
    /// Bookmark manager
    std::shared_ptr<BookmarkManager> m_bookmarkManager;
};

#endif // BOOKMARKBAR_H
