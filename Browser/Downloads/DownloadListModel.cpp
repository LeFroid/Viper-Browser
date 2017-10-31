#include "DownloadListModel.h"
#include "DownloadManager.h"

DownloadListModel::DownloadListModel(DownloadManager *manager, QObject *parent) :
    QAbstractListModel(parent),
    m_manager(manager)
{
}

int DownloadListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 0;

    return m_manager->m_downloads.size();
}

QVariant DownloadListModel::data(const QModelIndex &index, int /*role*/) const
{
    if (!index.isValid())
        return QVariant();

    return QVariant();
}

Qt::ItemFlags DownloadListModel::flags(const QModelIndex &index) const
{
    if (index.row() < 0 || index.row() >= m_manager->m_downloads.size())
        return 0;

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | QAbstractListModel::flags(index);
}
