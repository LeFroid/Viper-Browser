#include "FaviconStore.h"
#include "NetworkAccessManager.h"
#include "URL.h"

#include <array>
#include <QBuffer>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#include <QSqlError>
#include <QSqlRecord>
#include <QSvgRenderer>
#include <QDebug>

FaviconStore::FaviconStore(const QString &databaseFile) :
    DatabaseWorker(databaseFile, QLatin1String("Favicons")),
    m_originMap(),
    m_iconDataMap(),
    m_webPageMap(),
    m_favicons(),
    m_newFaviconID(1),
    m_newDataID(1),
    m_queryMap()
{
}

FaviconStore::~FaviconStore()
{
    //save();
}

int FaviconStore::getFaviconId(const QUrl &url)
{
    auto it = m_webPageMap.find(url);
    if (it != m_webPageMap.end())
        return *it;

    QSqlQuery iconQuery(m_database);
    iconQuery.prepare(QLatin1String("SELECT FaviconID FROM FaviconMap WHERE PageURL LIKE (:url)"));

    QString searchTemplate("%%1%");
    std::array<QString, 2> searchTerms = { searchTemplate.arg(url.host()),
                                           searchTemplate.arg(URL(url).getSecondLevelDomain()) };
    for (size_t i = 0; i < searchTerms.size(); ++i)
    {
        iconQuery.bindValue(QLatin1String(":url"), searchTerms.at(i));
        if (iconQuery.exec() && iconQuery.first())
        {
            bool ok = false;
            int result = iconQuery.value(0).toInt(&ok);
            if (ok)
                return result;
        }
    }

    return -1;
}

int FaviconStore::getFaviconIdForIconUrl(const QUrl &url)
{
    for (const auto &it : m_originMap)
    {
        if (url.matches(it.second, QUrl::RemoveScheme | QUrl::RemoveQuery | QUrl::RemoveFragment))
            return it.first;
    }

    int id = m_newFaviconID++;
    QSqlQuery *query = m_queryMap.at(StoredQuery::InsertFavicon).get();
    query->bindValue(QLatin1String(":iconId"), id);
    query->bindValue(QLatin1String(":url"), url);
    if (!query->exec())
        qDebug() << "In FaviconStore::getFaviconIdForIconUrl - could not add favicon metadata to Favicons table. Message: "
                 << query->lastError().text();
    return id;
}

QByteArray FaviconStore::getIconData(int faviconId) const
{
    auto it = m_iconDataMap.find(faviconId);
    if (it != m_iconDataMap.end())
        return it->second.iconData;

    return QByteArray();
}

FaviconData &FaviconStore::getDataRecord(int faviconId)
{
    auto it = m_iconDataMap.find(faviconId);
    if (it != m_iconDataMap.end())
        return it->second;

    FaviconData iconData;
    iconData.faviconId = faviconId;
    iconData.id = m_newDataID++;

    QSqlQuery *query = m_queryMap.at(StoredQuery::InsertIconData).get();
    query->bindValue(QLatin1String(":dataId"), iconData.id);
    query->bindValue(QLatin1String(":iconId"), iconData.faviconId);
    query->bindValue(QLatin1String(":data"), iconData.iconData);
    if (!query->exec())
        qDebug() << "In FaviconStore::getDataRecord - could not add favicon icon data to FaviconData table. Message: "
                 << query->lastError().text();

    auto result = m_iconDataMap.emplace(std::make_pair(faviconId, iconData));
    return result.first->second;
}

void FaviconStore::saveDataRecord(FaviconData &dataRecord)
{
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("UPDATE FaviconData SET Data = (:data) WHERE DataID = (:dataId)"));
    query.bindValue(QLatin1String(":data"), dataRecord.iconData);
    query.bindValue(QLatin1String(":dataId"), dataRecord.id);
    if (!query.exec())
        qDebug() << "In FaviconStore::saveDataRecord - could not update favicon data. Message: "
                 << query.lastError().text();
}

void FaviconStore::addPageMapping(const QUrl &webPageUrl, int faviconId)
{
    auto it = m_webPageMap.find(webPageUrl);
    if (it != m_webPageMap.end())
    {
        if (it.value() == faviconId)
            return;

        *it = faviconId;
    }
    else
        m_webPageMap.insert(webPageUrl, faviconId);

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("INSERT OR IGNORE INTO FaviconMap(PageURL, FaviconID) VALUES(:pageUrl, :iconId)"));
    query.bindValue(QLatin1String(":pageUrl"), webPageUrl);
    query.bindValue(QLatin1String(":iconId"), faviconId);
    if (!query.exec())
        qDebug() << "In FaviconStore::addPageMapping - could not update web page mapping in DB. Message: "
                 << query.lastError().text();
}

void FaviconStore::setupQueries()
{
    m_queryMap.clear();

    auto savedQuery = std::make_unique<QSqlQuery>(m_database);
    savedQuery->prepare(QLatin1String("INSERT OR REPLACE INTO Favicons(FaviconID, URL) VALUES (:iconId, :url)"));
    m_queryMap[StoredQuery::InsertFavicon] = std::move(savedQuery);

    savedQuery = std::make_unique<QSqlQuery>(m_database);
    savedQuery->prepare(QLatin1String("INSERT OR REPLACE INTO FaviconData(DataID, FaviconID, Data) VALUES (:dataId, :iconId, :data)"));
    m_queryMap[StoredQuery::InsertIconData] = std::move(savedQuery);

    savedQuery = std::make_unique<QSqlQuery>(m_database);
    savedQuery->prepare(QLatin1String("SELECT URL FROM Favicons WHERE FaviconID = (SELECT m.FaviconID FROM FaviconMap m WHERE m.PageURL = (:url))"));
    m_queryMap[StoredQuery::FindIconExactURL] = std::move(savedQuery);

    savedQuery = std::make_unique<QSqlQuery>(m_database);
    savedQuery->prepare(QLatin1String("SELECT URL FROM Favicons WHERE FaviconID IN (SELECT m.FaviconID FROM FaviconMap m WHERE m.PageURL LIKE (:url))"));
    m_queryMap[StoredQuery::FindIconLikeURL] = std::move(savedQuery);
}

bool FaviconStore::hasProperStructure()
{
    return hasTable(QLatin1String("Favicons"))
            && hasTable(QLatin1String("FaviconData"))
            && hasTable(QLatin1String("FaviconMap"));
}

void FaviconStore::setup()
{   
    // Setup table structures
    QSqlQuery query(m_database);
    query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS Favicons(FaviconID INTEGER PRIMARY KEY, URL TEXT UNIQUE)"));
    query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS FaviconData(DataID INTEGER PRIMARY KEY, FaviconID INTEGER NOT NULL, Data BLOB, "
               "FOREIGN KEY(FaviconID) REFERENCES Favicons(FaviconID))"));
    query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS FaviconMap(MapID INTEGER PRIMARY KEY, PageURL TEXT UNIQUE, FaviconID INTEGER NOT NULL, "
               "FOREIGN KEY(FaviconID) REFERENCES Favicons(FaviconID))"));

    // Create indices
    query.exec(QLatin1String("CREATE INDEX favicons_url ON Favicons(URL)"));
    query.exec(QLatin1String("CREATE INDEX favicon_data_data_id ON FaviconData(DataID)"));
    query.exec(QLatin1String("CREATE INDEX favicon_data_foreign_id ON FaviconData(FaviconID)"));
    query.exec(QLatin1String("CREATE INDEX favicon_map_url ON FaviconMap(PageURL)"));
    query.exec(QLatin1String("CREATE INDEX favicon_map_data_id ON FaviconMap(FaviconID)"));
}

void FaviconStore::load()
{
    setupQueries();

    QSqlQuery query(m_database);
    if (query.exec(QLatin1String("SELECT FaviconID, URL FROM Favicons")))
    {
        while (query.next())
        {
            int id = query.value(0).toInt();
            QUrl url = query.value(1).toUrl();
            m_originMap.emplace(std::make_pair(id, url));
        }
    }
    if (query.exec(QLatin1String("SELECT DataID, FaviconID, Data FROM FaviconData")))
    {
        while (query.next())
        {
            FaviconData data;
            data.id = query.value(0).toInt();
            data.faviconId = query.value(1).toInt();
            data.iconData = query.value(2).toByteArray();

            m_iconDataMap.emplace(std::make_pair(data.faviconId, data));
        }
    }
    if (query.exec(QLatin1String("SELECT PageURL, FaviconID FROM FaviconMap")))
    {
        while (query.next())
        {
            QUrl url = query.value(0).toUrl();
            int faviconId = query.value(1).toInt();
            m_webPageMap.insert(url, faviconId);
        }
    }

    // Fetch maximum favicon ID and data ID values so new entry IDs can be calculated with more ease
    if (query.exec(QLatin1String("SELECT MAX(FaviconID) FROM Favicons")) && query.first())
        m_newFaviconID = query.value(0).toInt() + 1;
    if (query.exec(QLatin1String("SELECT MAX(DataID) FROM FaviconData")) && query.first())
        m_newDataID = query.value(0).toInt() + 1;
}

void FaviconStore::save()
{
    /*
    // Prepare query to save icon:url mapping
    QSqlQuery queryIconMap(m_database);
    queryIconMap.prepare(QLatin1String("INSERT OR IGNORE INTO FaviconMap(PageURL, FaviconID) VALUES(:pageUrl, :iconId)"));

    // Iterate through favicon entries
    int iconID;
    for (auto it = m_favicons.cbegin(); it != m_favicons.cend(); ++it)
    {
        if (it->icon.isNull())
            continue;

        iconID = it->iconID;
        for (const auto page : it->urls)
        {
            queryIconMap.bindValue(QLatin1String(":pageUrl"), page);
            queryIconMap.bindValue(QLatin1String(":iconId"), iconID);
            if (!queryIconMap.exec())
                qDebug() << "[Error]: In FaviconStore::save() - Could not map URL to favicon in database. Message: "
                         << queryIconMap.lastError().text();
        }
    }*/
}
