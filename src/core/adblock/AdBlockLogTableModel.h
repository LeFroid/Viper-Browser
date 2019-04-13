#ifndef ADBLOCKLOGTABLEMODEL_H
#define ADBLOCKLOGTABLEMODEL_H

#include "AdBlockLog.h"
#include <QAbstractTableModel>
#include <vector>

namespace adblock
{

/**
 * @class LogTableModel
 * @brief Interacts with a table view to display the logs from the AdBlock system
 * @ingroup AdBlock
 */
class LogTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    /// Constructs the table model with a pointer to its parent
    explicit LogTableModel(QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    /// Returns the data associated at the index with the given role
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    /// Sets the log entry data to be shown in the table
    void setLogEntries(const std::vector<LogEntry> &entries);

private:
    /// Returns the element typemask as a formatted string ("type1[, type2, ..., typeN]")
    QString elementTypeToString(ElementType type) const;

private:
    /// Logs stored in the table model
    std::vector<LogEntry> m_logEntries;
};

}

#endif // ADBLOCKLOGTABLEMODEL_H
