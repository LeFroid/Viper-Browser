#ifndef BookmarkWidget_H
#define BookmarkWidget_H

#include "BookmarkManager.h"
#include <deque>
#include <QModelIndex>
#include <QUrl>
#include <QVector>
#include <QWidget>

namespace Ui {
class BookmarkWidget;
}

/**
 * @class BookmarkWidget
 * @brief Provides a graphical interface for managing the user's bookmarks
 * @ingroup Bookmarks
 */
class BookmarkWidget : public QWidget
{
    Q_OBJECT

    /// Options available from the import/export combo box
    enum class ComboBoxOption
    {
        NoAction   = 0,
        ImportHTML = 1,
        ExportHTML = 2
    };

public:
    /// Constructs the bookmark manager widget
    explicit BookmarkWidget(QWidget *parent = nullptr);

    /// BookmarkWidget destructor
    ~BookmarkWidget();

    /// Sets the pointer to the user's bookmark manager
    void setBookmarkManager(BookmarkManager *bookmarkManager);

protected:
    /// Called to adjust the proportions of the columns belonging to the table view
    virtual void resizeEvent(QResizeEvent *event) override;

signals:
    /// Signal for the browser to open a bookmark onto the current web page
    void openBookmark(const QUrl &link);

    /// Signal for the browser to open a bookmark into a new tab
    void openBookmarkNewTab(const QUrl &link, bool makeCurrent = false);

    /// Signal for the browser to open a bookmark into a new window
    void openBookmarkNewWindow(const QUrl &link);

private slots:
    /// Creates a context menu for the table view at the position of the cursor
    void onBookmarkContextMenu(const QPoint &pos);

    /// Creates a context menu for the folder view widget
    void onFolderContextMenu(const QPoint &pos);

    /// Called when the selected bookmark node in a given folder has changed from the previous index to the current index
    void onBookmarkSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

    /// Called when the user changes their bookmark folder selection
    void onFolderSelectionChanged(const QModelIndex &current, const QModelIndex &previous);

    /// Called when the index of the active item has changed in the "Import/Export bookmarks from/to HTML" combo box
    void onImportExportBoxChanged(int index);

    /// Emits the openBookmark signal with the link parameter set to the URL of the bookmark requested to be opened
    void openInCurrentPage();

    /// Emits the openBookmarkNewTab signal with the link parameter set to the URL of the bookmark requested to be opened
    void openInNewTab();

    /**
     * @brief Opens each bookmark belonging to the given folder in a new browser tab. Does not open
     *        bookmarks that belong to sub-folders of the parent folder
     * @param folder Pointer to the parent folder
     */
    void openAllBookmarksNewTabs(BookmarkNode *folder);

    /// Emits the openBookmarkNewWindow signal with the link parameter set to the URL of the bookmark requested to be opened
    void openInNewWindow();

    /// Called by the add new bookmark action
    void addBookmark();

    /// Called by the add new folder action
    void addFolder();

    /// Adds a top-level folder (ie direct descendant of the root folder)
    void addTopLevelFolder();

    /// Deletes the current bookmark selection
    void deleteBookmarkSelection();

    /// Deletes the current folder selection (ignoring the root folder if root is included)
    void deleteFolderSelection();

    /// Called when the text in the search bar is entered, searches for a bookmark or folder matching the given string
    void searchBookmarks();

    /// Resets the bookmark folder model
    void resetFolderModel();

    /// Begins a reset operation on the bookmark table model
    void beginResetTableModel();

    /// Ends a reset operation on the bookmark table model
    void endResetTableModel();

    /// Called when a bookmark folder has moved from one parent folder to another. If it was being displayed in the table model, the
    /// data must be updated
    void onFolderMoved(BookmarkNode *folder, BookmarkNode *updatedPtr);

    /// Called when the user wants to go back by one selection in the folder view
    void onClickBackButton();

    /// Called when the user wants to go forward by one selection in the folder view
    void onClickForwardButton();

    /// Updates any needed UI elements after a change has been made to the folder model
    void onFolderDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

    /// Updates any needed UI elements after a change has been made to the table model
    void onTableDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);

    /// Triggered when the Return or Enter key is pressed from the bookmark name line edit
    void onEditNodeName();

    /// Triggered when the Return or Enter key is pressed from the bookmark URL line edit
    void onEditNodeURL();

    /// Triggered when the Return or Enter key is pressed from the bookmark shortcut line edit
    void onEditNodeShortcut();

private:
    /// Returns a QUrl containing the location of the bookmark that the user has selected in the table view
    QUrl getUrlForSelection();

    /// Displays information about the given node at the bottom of the window
    void showInfoForNode(BookmarkNode *node);

private:
    /// Dialog's user interface elements
    Ui::BookmarkWidget *ui;

    /// Pointer to the user's bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// Pointer to the selected bookmark node
    BookmarkNode *m_currentNode;

    /// Stores the indices of previous selections made in the folder view. Used for the back button feature
    std::deque<QModelIndex> m_folderBackHistory;

    /// Stores the indices of active selections in the folder view when the "Back" button is clicked. Used for forward button feature
    std::deque<QModelIndex> m_folderForwardHistory;
};

#endif // BookmarkWidget_H
