#ifndef BOOKMARKFOLDERMODEL_H
#define BOOKMARKFOLDERMODEL_H

#include "BookmarkManager.h"
#include <memory>
#include <QAbstractItemModel>

/**
 * @class BookmarkFolderModel
 * @brief Model used for the bookmark manager's tree view, for
 *        displaying, adding and removing bookmark folders
 */
class BookmarkFolderModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    /// Constructs the bookmark folder model
    explicit BookmarkFolderModel(std::shared_ptr<BookmarkManager> bookmarkMgr, QObject *parent = nullptr);
    ~BookmarkFolderModel();

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

private:
    /// Returns the folder associated with the given model index, or the root folder if index is invalid
    BookmarkNode *getItem(const QModelIndex &index) const;

private:
    /// Root bookmark folder
    BookmarkNode *m_root;

    /// Bookmark manager
    std::shared_ptr<BookmarkManager> m_bookmarkMgr;
};

#endif // BOOKMARKFOLDERMODEL_H
