#ifndef FAVICONSTORAGE_H
#define FAVICONSTORAGE_H

#include "DatabaseWorker.h"

#include <QHash>
#include <QIcon>
#include <QSet>
#include <QString>
#include <QUrl>

class NetworkAccessManager;
class QNetworkReply;
class QSqlQuery;

/// Stores information about a favicon
struct FaviconInfo
{
    /// The icon's FaviconID from the Favicons table (which stores the URL of the favicon on the host server)
    int iconID;

    /// The icon's DataID from the FaviconData table (used to access the icon)
    int dataID;

    /// The favicon
    QIcon icon;

    /// Set of URLs the user has visited in the most recent session that use the favicon
    QSet<QString> urlSet;
};

/**
 * @class FaviconStorage
 * @brief Maintains a record of favicons from websites frequented by the user
 */
class FaviconStorage : public QObject, private DatabaseWorker
{
    friend class DatabaseFactory;

    Q_OBJECT

public:
    /**
     * @brief FaviconStorage Constructs the Favicon storage object
     * @param databaseFile Path of the favicon database file
     */
    FaviconStorage(const QString &databaseFile, QObject *parent = nullptr);

    /// Destroys the favicon storage object, saving data to the favicon database
    virtual ~FaviconStorage();

    /// Returns the favicon associated with the given URL if found in the database, otherwise
    /// returns an empty icon
    QIcon getFavicon(const QUrl &url) const;

    /// Updates the database (if applicable) about a favicon existing at the given location and being
    /// referred to by the page URL
    void updateIcon(const QString &iconHRef, QString pageUrl);

private slots:
    /// Called after the request for a favicon has been completed
    void onReplyFinished();

private:
    /// Converts the base64-encoded byte array into a QIcon
    QIcon iconFromBase64(QByteArray data);

    /// Returns the base64 encoding of the given icon
    QByteArray iconToBase64(QIcon icon);

    /// Saves the specific favicon with its URL and data structure into the database
    void saveToDB(const QString &faviconUrl, const FaviconInfo &favicon);

protected:
    /// Returns true if the favicon database contains the table structure(s) needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Sets initial table structures of the database
    void setup() override;

    /// Saves information to the database
    void save() override;

    /// Loads records from the database
    void load() override;

private:
    /// Network access manager - used to fetch favicons
    NetworkAccessManager *m_accessMgr;

    /// Network reply pointer for favicon requests
    QNetworkReply *m_reply;

    /// Hash map of favicon URLs to their data in a \ref FaviconInfo structure
    QHash<QString, FaviconInfo> m_favicons;

    /// Used when adding new records to the favicon table
    int m_newFaviconID;

    /// Used when adding new records to the favicon data table
    int m_newDataID;

    /// Prepared statements for inserting a new favicon into the database
    QSqlQuery *m_queryInsertFavicon, *m_queryInsertIconData;
};

#endif // FAVICONSTORAGE_H
