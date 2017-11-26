#ifndef BOOKMARKTABLEMODEL_H
#define BOOKMARKTABLEMODEL_H

#include "BookmarkManager.h"
#include <memory>
#include <QAbstractTableModel>

class QMimeData;

/**
 * @class BookmarkTableModel
 * @brief Model that displays the bookmarks belonging to whatever
 *        bookmark folder that is being selected in the bookmark folder view
 */
class BookmarkTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// Constructs the bookmark table model, given the shared pointer to the bookmark manager
    explicit BookmarkTableModel(std::shared_ptr<BookmarkManager> bookmarkMgr, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
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

    /// Sets the current folder to display in the table
    void setCurrentFolder(BookmarkNode *folder);

    /// Returns a pointer to the current folder being used by the model
    BookmarkNode *getCurrentFolder() const;

signals:
    /// Emitted when a folder has been moved. Used so that the BookmarkFolderModel can update itself appropriately
    void movedFolder();

private:
    /// Bookmark manager
    std::shared_ptr<BookmarkManager> m_bookmarkMgr;

    /// Current folder
    BookmarkNode *m_folder;
};

#endif // BOOKMARKTABLEMODEL_H
