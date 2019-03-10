#ifndef FAVICONTYPES_H
#define FAVICONTYPES_H

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
struct FaviconData
{
    /// Unique data identifier
    int id;

    /// Unique identifier of the favicon
    int faviconId;

    /// Icon data in the form of a byte array
    QByteArray iconData;

    /// Default constructor
    FaviconData() : id(0), faviconId(0), iconData() {}
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

// old structure:

/// Stores information about a favicon
struct FaviconInfo
{
    /// The icon's FaviconID from the Favicons table (which stores the URL of the favicon on the host server)
    int iconID;

    /// The icon's DataID from the FaviconData table (used to access the icon)
    int dataID;

    // The favicon
    QIcon icon;

    /// Icon in the form of a byte array
    QByteArray iconData;

    /// Set of URLs the user has visited in the most recent session that use the favicon
    QSet<QString> urls;

    /// Default constructor
    FaviconInfo() : iconID(-1), dataID(-1), /*icon(),*/ iconData(), urls() {}
};

#endif // FAVICONTYPES_H

