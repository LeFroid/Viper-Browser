#include <QDateTime>
#include "DetailedCookieTableModel.h"

DetailedCookieTableModel::DetailedCookieTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_cookie()
{
}

int DetailedCookieTableModel::rowCount(const QModelIndex &/*parent*/) const
{
    return 6;
}

int DetailedCookieTableModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 2;
}

QVariant DetailedCookieTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return QVariant();

    if (index.column() == 0)
    {
        switch (index.row())
        {
            case 0: return "Name:";
            case 1: return "Content:";
            case 2: return "Domain:";
            case 3: return "Path:";
            case 4: return "Send For:";
            case 5: return "Expires:";
        }
    }
    else if (index.column() == 1 && m_cookie.domain().size())
    {
        switch (index.row())
        {
            case 0: return m_cookie.name();
            case 1: return m_cookie.value();
            case 2: return m_cookie.domain();
            case 3: return m_cookie.path();
            case 4: return (m_cookie.isSecure() ? "Encrypted connections only" : "Any type of connection");
            case 5: return (m_cookie.isSessionCookie() ? "At the end of this session" : m_cookie.expirationDate().toString());
        }
    }

    return QVariant();
}

void DetailedCookieTableModel::setCookie(const QNetworkCookie &cookie)
{
    beginResetModel();
    m_cookie = cookie;
    endResetModel();
}
