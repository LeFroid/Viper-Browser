#ifndef BOOKMARKTABLEMODEL_H
#define BOOKMARKTABLEMODEL_H

#include "BookmarkNode.h"

#include <QAbstractTableModel>
#include <vector>

class BookmarkManager;
class QMimeData;

/**
 * @class BookmarkTableModel
 * @brief Model that displays the bookmarks belonging to the
 *        bookmark folder that is being selected in the bookmark folder view
 */
class BookmarkTableModel : public QAbstractTableModel
{
    friend class BookmarkWidget;

    Q_OBJECT

public:
    /// Constructs the bookmark table model, given a pointer to the bookmark manager
    explicit BookmarkTableModel(BookmarkManager *bookmarkMgr, QObject *parent = nullptr);

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

    /// Returns true if the model is displaying the results of a search operation, false if else
    bool isInSearchMode() const;

Q_SIGNALS:
    /// Emitted when a folder has been moved. Used so that the BookmarkFolderModel can update itself appropriately
    void movedFolder();

public Q_SLOTS:
    /// Searches for bookmarks with either a page title or domain containing the given string, displaying
    /// the matching results in the model
    void searchFor(const QString &text);

protected:
    /// Returns a pointer to the bookmark at the given row
    BookmarkNode *getBookmark(int row) const;

private:
    /// Bookmark manager
    BookmarkManager *m_bookmarkMgr;

    /// Current folder
    BookmarkNode *m_folder;

    /// True if the model is displaying the results of a bookmark search, false if else
    bool m_searchModeOn;

    /// Contains bookmark nodes that matched the criteria of a search query. Displayed by the model when search mode flag is enabled
    std::vector<BookmarkNode*> m_searchResults;
};

#endif // BOOKMARKTABLEMODEL_H
