#include "AdBlockModel.h"
#include "AdBlockManager.h"

AdBlockModel::AdBlockModel(AdBlockManager *parent) :
    QAbstractTableModel(parent),
    m_adBlockManager(parent)
{
}

QVariant AdBlockModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return QString("On");
            case 1: return QString("Name");
            case 2: return QString("Last Update");
            case 3: return QString("Source");
            default: return QVariant();
        }
    }

    return QVariant();
}

int AdBlockModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_adBlockManager->getNumSubscriptions();
}

int AdBlockModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant AdBlockModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return QVariant();

    const AdBlockSubscription *sub = m_adBlockManager->getSubscription(index.row());
    if (sub == nullptr)
        return QVariant();

    // Column is 0 for checkbox state
    if(index.column() == 0 && role == Qt::CheckStateRole)
    {
        return (sub->isEnabled() ? Qt::Checked : Qt::Unchecked);
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    switch (index.column())
    {
        case 1: return sub->getName();
        case 2: return sub->getLastUpdate().toString();
        case 3: return sub->getSourceUrl().toString(QUrl::FullyEncoded);
        default: return QVariant();
    }
}

Qt::ItemFlags AdBlockModel::flags(const QModelIndex& index) const
{
    if (index.column() == 0)
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    return QAbstractTableModel::flags(index);
}

bool AdBlockModel::setData(const QModelIndex &index, const QVariant &/*value*/, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole)
    {
        m_adBlockManager->toggleSubscriptionEnabled(index.row());
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

bool AdBlockModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();

    return true;
}
