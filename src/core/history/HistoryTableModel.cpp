#include "HistoryTableModel.h"
#include "HistoryManager.h"
#include "FaviconManager.h"

#include <utility>

HistoryTableModel::HistoryTableModel(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QAbstractTableModel(parent),
    m_historyManager(serviceLocator.getServiceAs<HistoryManager>("HistoryManager")),
    m_faviconManager(serviceLocator.getServiceAs<FaviconManager>("FaviconManager")),
    m_targetDate(),
    m_loadedDate(),
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
            case 0: return tr("Name");
            case 1: return tr("Location");
            case 2: return tr("Date");
        }
    }

    return QVariant();
}

int HistoryTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_history.size());
}

int HistoryTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // Fixed number of columns
    return 3;
}

bool HistoryTableModel::canFetchMore(const QModelIndex &/*parent*/) const
{
    return m_targetDate < m_loadedDate;
}

void HistoryTableModel::fetchMore(const QModelIndex &/*parent*/)
{
    qint64 daysLeft = m_targetDate.daysTo(m_loadedDate);
    if (daysLeft <= 0)
    {
        m_loadedDate = m_targetDate;
        return;
    }
    m_historyManager->getHistoryBetween(m_loadedDate.addDays(-1), m_loadedDate,
                                    std::bind(&HistoryTableModel::onHistoryFetched, this, std::placeholders::_1));
}

void HistoryTableModel::onHistoryFetched(std::vector<URLRecord> &&entries)
{
    QDateTime nextLoadedDate = m_loadedDate.addDays(-1);

    QMap<qint64, int> tmpVisitInfo; // Used to sort visits by date
    for (auto &it : entries)
    {
        // Load entry into common entry list, then specific visits into a temporary map for sorting
        HistoryTableItem tableItem;
        tableItem.Title = it.getTitle();
        tableItem.URL = it.getUrl().toString();
        tableItem.Favicon = m_faviconManager->getFavicon(it.getUrl()).pixmap(16, 16);
        m_commonData.push_back(tableItem);

        int itemIndex = static_cast<int>(m_commonData.size()) - 1;
        for (const auto &visit : it.getVisits())
        {
            auto time = visit.toMSecsSinceEpoch();
            if (!tmpVisitInfo.contains(time))
                tmpVisitInfo.insert(time, itemIndex);
        }
    }

    int currentRowCount = rowCount();
    beginInsertRows(QModelIndex(), currentRowCount, currentRowCount + tmpVisitInfo.size() - 1);

    // Insert history entries into m_history sorted by most recently visited
    QMapIterator<qint64, int> mapIt(tmpVisitInfo);
    mapIt.toBack();
    while (mapIt.hasPrevious())
    {
        mapIt.previous();
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(mapIt.key());

        HistoryTableRow row;
        row.ItemIndex = mapIt.value();
        row.VisitString = dateTime.toString(QStringLiteral("MMMM d yyyy, h:mm ap"));
        m_history.push_back(row);
    }

    m_loadedDate = nextLoadedDate;
    endInsertRows();
}

QVariant HistoryTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_history.size()))
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
            else if (role == Qt::DecorationRole)
                return itemData.Favicon;
            else if (role == Qt::SizeHintRole)
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

void HistoryTableModel::loadFromDate(const QDateTime &date)
{
    if (!date.isValid())
        return;

    beginResetModel();
    m_targetDate = date;

    // Set loaded date to a time in the future, as fetchMore() will grab history items one day at a time
    QDateTime tomorrow = QDateTime(QDate::currentDate(), QTime(0, 0));
    m_loadedDate = tomorrow.addDays(1);

    // Clear old model data
    m_commonData.clear();
    m_history.clear();

    endResetModel();
}
