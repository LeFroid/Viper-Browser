#ifndef FAVICONTYPES_H
#define FAVICONTYPES_H

#include "SQLiteWrapper.h"
#include "../database/bindings/QtSQLite.h"

#include <QIcon>

#include <unordered_map>

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QSet>
#include <QUrl>

/// Stores the metadata of single favicon - its unique ID, and its download URL
struct FaviconOrigin
{
    /// Unique identifier of the favicon
    int id;

    /// URL from which this icon was downloaded
    QUrl url;

    /// Default constructor
    FaviconOrigin() : id(0), url() {}
};

/// Stores encoded icon data for a specific favicon
struct FaviconData final : public sqlite::Row
{
    /// Unique data identifier
    int id;

    /// Unique identifier of the favicon
    int faviconId;

    /// Icon data in the form of a byte array
    QByteArray iconData;

    /// Default constructor
    FaviconData() : id(0), faviconId(0), iconData() {}

    /// Marshals the data into a prepared statement
    void marshal(sqlite::PreparedStatement &stmt) const override
    {
        stmt << id
             << faviconId
             << iconData;
    }

    /// Reads the data from a query result into this entity
    void unmarshal(sqlite::PreparedStatement &stmt) override
    {
        stmt >> id
             >> faviconId
             >> iconData;
    }
};

/// Mapping of specific web pages to their favicon records
struct FaviconMap
{
    /// Unique identifier of this mapping
    int id;

    /// Unique identifier of the favicon
    int faviconId;

    /// Page that referred to a favicon
    QUrl pageUrl;

    /// Default constructor
    FaviconMap() : id(0), faviconId(0), pageUrl() {}
};

/// Represents the \ref FaviconOrigin structure as a map. Key = favicon Id, value = URL of icon
using FaviconOriginMap = std::unordered_map<int, QUrl>;

/// Represents the \ref FaviconData structure as a map. Key = favicon Id (same as FaviconOriginMap), value = FaviconData structure
using FaviconDataMap = std::unordered_map<int, FaviconData>;

/// Represents the \ref FaviconMap as a hash map. Key = URL of the web page, value = unique identifier of the favicon.
using WebPageIconMap = QHash<QUrl, int>;

#endif // FAVICONTYPES_H
