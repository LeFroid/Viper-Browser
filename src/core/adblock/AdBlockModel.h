#ifndef ADBLOCKMODEL_H
#define ADBLOCKMODEL_H

#include <QAbstractTableModel>

namespace adblock
{

class AdBlockManager;

/**
 * @class AdBlockModel
 * @ingroup AdBlock
 * @brief Acts as an abstraction to allow the user to view and modify their \ref AdBlockSubscription collection
 */
class AdBlockModel : public QAbstractTableModel
{
    friend class AdBlockManager;

    Q_OBJECT

public:
    /// Constructs the AdBlockModel with the given parent object
    explicit AdBlockModel(AdBlockManager *parent = nullptr);

    /// Displays header information for the given section, header orientation and the header role
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /// Returns the number of rows (subscriptions) in the model
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the number of columns of subscription information
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the data stored at the given index
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /// Allow user to modify first column (enabled/disabled checkbox)
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /// Sets the data at the given index.
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    /// Inserts subscriptions into the model
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    /// Removes subscriptions from the model and the user's locaol storage
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

private:
    /// Pointer to the ad block manager
    AdBlockManager *m_adBlockManager;
};

}

#endif // ADBLOCKMODEL_H
