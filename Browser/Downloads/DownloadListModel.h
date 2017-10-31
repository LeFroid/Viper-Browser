#ifndef DOWNLOADLISTMODEL_H
#define DOWNLOADLISTMODEL_H

#include <QAbstractListModel>

class DownloadManager;

/**
 * @class DownloadListModel
 * @brief Models the download items onto the download manager's list view
 */
class DownloadListModel : public QAbstractListModel
{
    friend class DownloadManager;

    Q_OBJECT

public:
    /// DownloadListModel constructor
    explicit DownloadListModel(DownloadManager *manager, QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // DownloadItem flags
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    /// Download manager
    DownloadManager *m_manager;
};

#endif // DOWNLOADLISTMODEL_H
