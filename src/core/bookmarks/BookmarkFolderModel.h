#ifndef BOOKMARKFOLDERMODEL_H
#define BOOKMARKFOLDERMODEL_H

#include "BookmarkNode.h"

#include <QAbstractItemModel>

class BookmarkManager;

/**
 * @class BookmarkFolderModel
 * @brief Model used for the bookmark manager's tree view, for
 *        displaying, adding and removing bookmark folders
 * @ingroup Bookmarks
 */
class BookmarkFolderModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /// Constructs the bookmark folder model
    explicit BookmarkFolderModel(BookmarkManager *bookmarkMgr, QObject *parent = nullptr);

    /// Default destructor
    ~BookmarkFolderModel() = default;

    // Basic functionality:
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable:
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    // Drag and drop actions:
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDropActions() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    /// Returns the folder associated with the given model index, or the root folder if index is invalid
    BookmarkNode *getItem(const QModelIndex &index) const;

Q_SIGNALS:
    /// Emitted when one or more bookmarks are about to be dropped from one folder into another, so that the \ref BookmarkTableModel can update its data
    void beginMovingBookmarks();

    /// Emitted when one or more bookmarks are finished being dropped from one folder into another, so that the \ref BookmarkTableModel can update its data
    void endMovingBookmarks();

    /// Emitted when a folder was moved to a new parent node, so the \ref BookmarkTableModel can update its data if necessary
    void movedFolder(BookmarkNode *folder, BookmarkNode *updatedPtr);

private:
    /// Root bookmark folder
    BookmarkNode *m_root;

    /// Bookmarks Bar node - stored as a member because it cannot have editable flag when flags(...) is called
    BookmarkNode *m_bookmarksBar;

    /// Bookmark manager
    BookmarkManager *m_bookmarkMgr;
};

#endif // BOOKMARKFOLDERMODEL_H
