#ifndef FOLDERNAVIGATOR_H
#define FOLDERNAVIGATOR_H

#include "FolderNavigationAction.h"
#include <deque>

#include <QModelIndex>
#include <QObject>

class BookmarkTableModel;
class BookmarkWidget;
class QTreeView;

/**
 * @class FolderNavigator
 * @brief Facilitates the "Go Back" and "Go Forward" history-related
 *        actions in the \ref BookmarkWidget
 * @ingroup Bookmarks
 */
class FolderNavigator : public QObject
{
    Q_OBJECT

public:
    /// Constructs the navigator, given the pointer to its parent UI
    explicit FolderNavigator(BookmarkWidget *parentUi, BookmarkTableModel *tableModel, QTreeView *folderTree);

    /// Returns true if there is at least one navigation action that can be reversed. False otherwise.
    bool canGoBack() const;

    /// Returns true if there is at least one navigation action that can be redone. False otherwise.
    bool canGoForward() const;

    /// Clears the current navigation history, starting again with a blank slate
    void clear();

    /// Initializes the folder navigation history at the given root index
    void initialize(const QModelIndex &root);

Q_SIGNALS:
    /// Emitted whenever the navigation history has been updated.
    void historyChanged();

public Q_SLOTS:
    /// Performs an "undo" operation, or go back in history by 1
    void goBack();

    /// Performs a "redo" operation, or go forward in history by 1
    void goForward();

    /// Handles a new user-initiated navigation event. This clears any existing forward history
    void onFolderActivated(const QModelIndex &current, const QModelIndex &previous);

private:
    /// Executes the command at the given index
    void executeAt(std::size_t index);

private:
    /// Index of the most recently executed navigation action (ie "current")
    std::size_t m_indexHead;

    /// Flag indicating whether or not an undo/redo operation is in progress
    bool m_inProgress;

    /// Contains a record of every navigation action made in the current session
    std::deque<FolderNavigationAction> m_history;

    /// Points to the parent user interface
    BookmarkWidget *m_bookmarkUi;

    /// Points to the table model. Needed to pass on to the navigation commands.
    BookmarkTableModel *m_tableModel;

    /// Points to the folder tree view
    QTreeView *m_treeView;
};

#endif // FOLDERNAVIGATOR_H
