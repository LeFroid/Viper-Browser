#include <QNetworkAccessManager>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

#include "BrowserApplication.h"
#include "CookieTableModel.h"
#include "NetworkAccessManager.h"

CookieTableModel::CookieTableModel(QObject *parent, QWebEngineCookieStore *cookieStore) :
    QAbstractTableModel(parent),
    m_cookieStore(cookieStore),
    m_checkedState(),
    m_cookies(),
    m_searchResults(),
    m_searchModeOn(false)
{
    loadCookies();

    connect(m_cookieStore, &QWebEngineCookieStore::cookieAdded, this, &CookieTableModel::onCookieAdded);
    connect(m_cookieStore, &QWebEngineCookieStore::cookieRemoved, this, &CookieTableModel::onCookieRemoved);
}

QVariant CookieTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return QVariant();
            case 1: return QString("Domain");
            case 2: return QString("Name");
        }
    }

    return QVariant();
}

int CookieTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    if (m_searchModeOn)
        return m_searchResults.size();

    return m_cookies.size();
}

int CookieTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return 3;
}

QVariant CookieTableModel::data(const QModelIndex &index, int role) const
{
    // Check if column is 0 for checkbox state
    if(index.column() == 0 && role == Qt::CheckStateRole)
    {
        if (m_checkedState.size() > index.row())
            return m_checkedState.at(index.row());
        return Qt::Unchecked;
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    if (!index.isValid())
        return QVariant();

    if (m_searchModeOn && index.row() < m_searchResults.size())
    {
        QNetworkCookie cookie = m_searchResults.at(index.row());
        switch (index.column())
        {
            case 1: return cookie.domain();
            case 2: return cookie.name();
            default: return QVariant();
        }
    }

    if (index.row() < m_cookies.size())
    {
        QNetworkCookie cookie = m_cookies.at(index.row());
        switch (index.column())
        {
            case 1: return cookie.domain();
            case 2: return cookie.name();
        }
    }
    return QVariant();
}

Qt::ItemFlags CookieTableModel::flags(const QModelIndex& index) const
{
    if (index.column() == 0)
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    return QAbstractTableModel::flags(index);
}

bool CookieTableModel::setData(const QModelIndex &index, const QVariant &/*value*/, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole)
    {
        int val = m_checkedState.at(index.row());
        m_checkedState[index.row()] = (val == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

bool CookieTableModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();
    return true;
}

bool CookieTableModel::insertColumns(int column, int count, const QModelIndex &parent)
{
    beginInsertColumns(parent, column, column + count - 1);
    endInsertColumns();
    return true;
}

bool CookieTableModel::removeRows(int row, int count, const QModelIndex &parent)
{
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
    {
        QNetworkCookie cookie = (m_searchModeOn ? m_searchResults.at(row + i) : m_cookies.at(row + i));
        m_cookieStore->deleteCookie(cookie);
        if (m_searchModeOn)
            m_searchResults.removeAt(row + i);
        else
            m_cookies.removeAt(row + i);
        m_checkedState.removeAt(row + i);
    }
    endRemoveRows();
    return true;
}

bool CookieTableModel::removeColumns(int column, int count, const QModelIndex &parent)
{
    beginRemoveColumns(parent, column, column + count - 1);
    endRemoveColumns();
    return true;
}

QNetworkCookie CookieTableModel::getCookie(const QModelIndex &index) const
{
    if (m_searchModeOn)
    {
        if (index.row() >= m_searchResults.size())
            return QNetworkCookie();
        return m_searchResults.at(index.row());
    }

    if (index.row() >= m_cookies.size())
        return QNetworkCookie();

    return m_cookies.at(index.row());
}

const QList<int> &CookieTableModel::getCheckedStates() const
{
    return m_checkedState;
}

void CookieTableModel::searchFor(const QString &text)
{
    // Clear existing rows to reset view
    //removeRows(0, rowCount());
    beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
    endRemoveRows();

    // If empty string, return to normal view
    if (text.isEmpty())
    {
        m_searchModeOn = false;
        loadCookies();
        for (int i = 0; i < m_checkedState.size(); ++i)
            m_checkedState[i] = Qt::Unchecked;
        insertRows(0, rowCount());
        m_searchResults.clear();
        return;
    }

    // Reset any previous search results
    m_searchResults.clear();

    // Search for cookies with a name or domain value that match the search parameter
    QByteArray textBytesLower = text.toLower().toUtf8();
    for (const QNetworkCookie &cookie : qAsConst(m_cookies))
    {
        if (cookie.name().toLower().contains(textBytesLower)
                || cookie.domain().contains(text, Qt::CaseInsensitive))
        {
            m_searchResults.append(cookie);
        }
    }

    // Enable search mode
    m_searchModeOn = true;

    // Insert the new rows into the model
    insertRows(0, rowCount());
}

void CookieTableModel::loadCookies()
{
    int checkboxSize = m_checkedState.size();
    int cookieSize = m_cookies.size();
    if (checkboxSize < m_cookies.size())
    {
        for (int i = 0; i < cookieSize - checkboxSize; ++i)
            m_checkedState.append(Qt::Unchecked);
    }
}

void CookieTableModel::eraseCookies()
{
    int numCookies = m_cookies.size();
    m_cookies.clear();
    m_checkedState.clear();
    emit dataChanged(index(0, 0), index(numCookies - 1, 2));
}

void CookieTableModel::onCookieAdded(const QNetworkCookie &cookie)
{
    int rowNum = m_cookies.size();
    beginInsertRows(QModelIndex(), rowNum, rowNum);;
    m_cookies.append(cookie);
    endInsertRows();
}

void CookieTableModel::onCookieRemoved(const QNetworkCookie &cookie)
{
    int index = m_cookies.indexOf(cookie);
    if (index < 0)
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_cookies.removeAt(index);
    endRemoveRows();
}
