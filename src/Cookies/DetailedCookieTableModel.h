#ifndef DETAILEDCOOKIETABLEMODEL_H
#define DETAILEDCOOKIETABLEMODEL_H

#include <QAbstractTableModel>
#include <QNetworkCookie>

class DetailedCookieTableModel : public QAbstractTableModel
{
    friend class CookieWidget;

    Q_OBJECT

public:
    explicit DetailedCookieTableModel(QObject *parent = nullptr);

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    /// Sets the cookie to be displayed by the model
    void setCookie(const QNetworkCookie &cookie);

private:
    /// Cookie being displayed by the table
    QNetworkCookie m_cookie;
};

#endif // DETAILEDCOOKIETABLEMODEL_H
