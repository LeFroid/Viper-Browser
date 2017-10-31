#include "BrowserApplication.h"
#include "HistoryTableModel.h"
#include "HistoryManager.h"
#include "FaviconStorage.h"

#include <QDebug>
HistoryTableModel::HistoryTableModel(HistoryManager *historyMgr, QObject *parent) :
    QAbstractTableModel(parent),
    m_historyMgr(historyMgr),
    m_commonData(),
    m_history()
{
}

QVariant HistoryTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return "Name";
            case 1: return "Location";
            case 2: return "Date";
        }
    }

    return QVariant();
}

int HistoryTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_history.size();
}

int HistoryTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // Fixed number of columns
    return 3;
}

QVariant HistoryTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_history.size())
        return QVariant();

    if (index.column() > 0 && role != Qt::DisplayRole)
        return QVariant();

    const HistoryTableRow &item = m_history.at(index.row());
    const HistoryTableItem &itemData = m_commonData.at(item.ItemIndex);
    switch (index.column())
    {
        // Name / favicon column
        case 0:
        {
            if (role == Qt::DisplayRole)
                return itemData.Title;

            if (role == Qt::DecorationRole)
                return itemData.Favicon;

            if (role == Qt::SizeHintRole)
                return itemData.Favicon.size();
            break;
        }
        // URL column
        case 1: return itemData.URL;
        // Visit string
        case 2: return item.VisitString;
    }

    return QVariant();
}

QUrl HistoryTableModel::getIndexURL(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_history.size())
        return QUrl();

    const HistoryTableItem &itemData = m_commonData.at(m_history.at(index.row()).ItemIndex);
    return QUrl::fromUserInput(itemData.URL);
}

void HistoryTableModel::loadFromDate(const QDateTime &date)
{
    if (!date.isValid())
        return;
    FaviconStorage *favicons = sBrowserApplication->getFaviconStorage();

    beginResetModel();

    // load data from history manager and use FaviconStorage to fetch icons
    m_commonData.clear();
    m_history.clear();
    QList<WebHistoryItem> entries = m_historyMgr->getHistoryFrom(date);
    QMap<qint64, int> tmpVisitInfo; // Used to sort visits by date
    for (auto it : entries)
    {
        // Load entry into common entry list, then specific visits into a temporary map for sorting
        HistoryTableItem tableItem;
        tableItem.Title = it.Title;
        tableItem.URL = it.URL.toString();
        tableItem.Favicon = favicons->getFavicon(tableItem.URL).pixmap(16, 16);
        m_commonData.append(tableItem);

        int itemIndex = m_commonData.size() - 1;
        for (auto visit : it.Visits)
            tmpVisitInfo.insert(visit.toMSecsSinceEpoch(), itemIndex);
    }

    // Insert history entries into m_history sorted by most recently visited
    QMapIterator<qint64, int> mapIt(tmpVisitInfo);
    mapIt.toBack();
    while (mapIt.hasPrevious())
    {
        mapIt.previous();
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(mapIt.key());

        HistoryTableRow row;
        row.ItemIndex = mapIt.value();
        row.VisitString = dateTime.toString("MMMM d yyyy, h:m ap");
        m_history.append(row);
    }

    endResetModel();
}
