#ifndef HISTORYTABLEMODEL_H
#define HISTORYTABLEMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>
#include <QMap>
#include <QPixmap>
#include <QUrl>

class HistoryManager;

/**
 * @struct HistoryTableItem
 * @brief Represents one item used by the \ref HistoryTableModel
 *        Items may be repeated several times in the table, so the data
 *        structure for an individual row, \ref HistoryTableRow will
 *        reference this structure by its index in a list
 */
struct HistoryTableItem
{
    /// Favicon of the page
    QPixmap Favicon;

    /// Title of the web page
    QString Title;

    /// URL of the page
    QString URL;
};

/**
 * @struct HistoryTableRow
 * @brief Represents an individual row of data in the \ref HistoryTableModel
 */
struct HistoryTableRow
{
    /// Index of the history table item
    int ItemIndex;

    /// Date/time of visit in string format
    QString VisitString;
};

/**
 * @class HistoryTableModel
 * @brief Loads browser history within a given range of dates into a table view
 */
class HistoryTableModel : public QAbstractTableModel
{
    Q_OBJECT

    friend class HistoryWidget;

public:
    explicit HistoryTableModel(HistoryManager *historyMgr, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /// Returns the URL associated with the given index, or an empty URL if index is invalid
    QUrl getIndexURL(const QModelIndex &index) const;

protected:
    /// Loads all history items beginning at the given date
    void loadFromDate(const QDateTime &date);

private:
    /// History manager
    HistoryManager *m_historyMgr;

    /// Common history data
    QList<HistoryTableItem> m_commonData;

    /// List of visited history items, ordered by most to least recent visit
    QList<HistoryTableRow> m_history;
};

#endif // HISTORYTABLEMODEL_H
