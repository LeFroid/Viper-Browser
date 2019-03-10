#ifndef FAVICONSTORAGE_H
#define FAVICONSTORAGE_H

#include "DatabaseWorker.h"
#include "FaviconTypes.h"
#include "LRUCache.h"

#include <map>
#include <memory>
#include <QHash>
#include <QIcon>
#include <QSet>
#include <QSqlQuery>
#include <QString>
#include <QUrl>

/**
 * @class FaviconStore
 * @brief Maintains a record of favicons from websites frequented by the user
 */
class FaviconStore : public DatabaseWorker
{
    friend class DatabaseFactory;

public:
    /**
     * @brief FaviconStore Constructs the Favicon storage object
     * @param databaseFile Path of the favicon database file
     */
    FaviconStore(const QString &databaseFile);

    /// Destroys the favicon storage object, saving data to the favicon database
    ~FaviconStore();

    /// Returns the Id of the favicon associated with the given URL
    int getFaviconId(const QUrl &url);

    /// Returns the identifier of the favicon associated with the given data URL (ie the URL of the icon itself)
    int getFaviconIdForIconUrl(const QUrl &url);

    /// Returns the icon data for the favicon with the given identifier, or an empty
    /// byte array if it could not be found
    QByteArray getIconData(int faviconId) const;

    /// Returns a reference to the icon data structure associated with the given favicon ID,
    /// inserting a new record if it was not found.
    FaviconData &getDataRecord(int faviconId);

    /// Saves the given record to the database
    void saveDataRecord(FaviconData &dataRecord);

    /// Maps the given web page to a favicon, referenced by its unique ID
    void addPageMapping(const QUrl &webPageUrl, int faviconId);

private:
    /// Instantiates the stored query objects
    void setupQueries();

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
    /// Used to access prepared database queries
    enum class StoredQuery
    {
        InsertFavicon,
        InsertIconData,
        FindIconExactURL,
        FindIconLikeURL
    };

private:
    /// Mapping of unique favicon IDs to their respective data URLs
    FaviconOriginMap m_originMap;

    /// Mapping of unique favicon IDs to their icon data containers
    FaviconDataMap m_iconDataMap;

    /// Mapping of visited URLs to their corresponding favicon IDs
    WebPageIconMap m_webPageMap;

    //==

    /// Hash map of favicon URLs to their data in a \ref FaviconInfo structure
    QHash<QUrl, FaviconInfo> m_favicons;

    /// Used when adding new records to the favicon table
    int m_newFaviconID;

    /// Used when adding new records to the favicon data table
    int m_newDataID;

    /// Map of query types to pointers of commonly used prepared statements
    std::map< StoredQuery, std::unique_ptr<QSqlQuery> > m_queryMap;

    /*
/// Represents the \ref FaviconOrigin structure as a map. Key = favicon Id, value = URL of icon
using FaviconOriginMap = std::map<int, QUrl>;

/// Represents the \ref FaviconData structure as a map. Key = favicon Id (same as FaviconOriginMap), value = FaviconData structure
using FaviconDataMap = std::map<int, FaviconData>;

/// Represents the \ref FaviconMap as a hash map. Key = URL of the web page, value = unique identifier of the favicon.
using WebPageIconMap = QHash<QUrl, int>;*/
};

#endif // FAVICONSTORAGE_H
