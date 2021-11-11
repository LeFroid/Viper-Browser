#ifndef COOKIETABLEMODEL_H
#define COOKIETABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QNetworkCookie>

class CookieJar;
class QWebEngineCookieStore;

/**
 * @class CookieTableModel
 * @brief Handles formatting of general cookie information in the \ref CookieWidget top table
 */
class CookieTableModel : public QAbstractTableModel
{
    friend class CookieWidget;

    Q_OBJECT

public:
    /// Constructs the cookie table model
    explicit CookieTableModel(QObject *parent, QWebEngineCookieStore *cookieStore);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Basic functionality:
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Editable
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    // Add data:
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool insertColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    // Remove data:
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    bool removeColumns(int column, int count, const QModelIndex &parent = QModelIndex()) override;

    /// Returns the cookie associated with the given index
    QNetworkCookie getCookie(const QModelIndex &index) const;

    /// Returns a list of each row and whether or not the row is checked
    const QList<int> &getCheckedStates() const;

public Q_SLOTS:
    /// Searches for cookies with either a name or domain containing the given string, displaying
    /// the matching subset of cookies in the model
    void searchFor(const QString &text);

    /// Reloads the local copy of the browser's cookies
    void loadCookies();

private Q_SLOTS:
    /// Called when the cookies have been erased
    void eraseCookies();

    /// Called when a cookie has been added to the cookie store
    void onCookieAdded(const QNetworkCookie &cookie);

    /// Called when a cookie has been removed from the cookie store
    void onCookieRemoved(const QNetworkCookie &cookie);

private:
    /// Cookie store
    QWebEngineCookieStore *m_cookieStore;

    /// Stores each row's checked state
    QList<int> m_checkedState;

    /// Stores a copy of the browser's cookies
    QList<QNetworkCookie> m_cookies;

    /// Stores a copy of the browser's cookies that match a search query
    QList<QNetworkCookie> m_searchResults;

    /// True if the model is displaying the results of a cookie search, false if else
    bool m_searchModeOn;
};

#endif // COOKIETABLEMODEL_H
