#ifndef FOLDERNAVIGATIONACTION_H
#define FOLDERNAVIGATIONACTION_H

#include <QModelIndex>

class BookmarkTableModel;
class BookmarkWidget;
class QTreeView;

/**
 * @class FolderNavigationAction
 * @brief Encapsulates the functionality of navigating between bookmark
 *        folders in the UI. Implemented per the command pattern to support
 *        undo() and redo() operations with relative ease.
 * @ingroup Bookmarks
 */
class FolderNavigationAction
{
public:
    /**
     * @brief Instantiates the action with a fixed set of parameters
     * @param index Folder index of the target folder (from \ref BookmarkFolderModel)
     * @param tableModel Table model pointer
     * @param treeView The folder tree view
     */
    explicit FolderNavigationAction(const QModelIndex &index,
                                    BookmarkTableModel *tableModel,
                                    BookmarkWidget *bookmarkUi,
                                    QTreeView *treeView);

    /**
     * @brief Navigates to the bookmark folder associated with this action.
     */
    void execute();

private:
    /// The folder model index
    QModelIndex m_index;

    /// Pointer to the table model, needed for both undo and execute
    BookmarkTableModel *m_tableModel;

    /// User interface
    BookmarkWidget *m_bookmarkUi;

    /// Folder navigation tree view
    QTreeView *m_treeView;
};

#endif // FOLDERNAVIGATIONACTION_H
