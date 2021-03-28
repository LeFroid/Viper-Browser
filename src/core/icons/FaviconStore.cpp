#include "CommonUtil.h"
#include "FaviconStore.h"
#include "URL.h"

#include <array>
#include <QDebug>

FaviconStore::FaviconStore(const QString &databaseFile) :
    DatabaseWorker(databaseFile),
    m_originMap(),
    m_iconDataMap(),
    m_webPageMap(),
    m_newFaviconID(1),
    m_newDataID(1),
    m_queryMap()
{
}

FaviconStore::~FaviconStore()
{
}

int FaviconStore::getFaviconId(const QUrl &url)
{
    if (url.isEmpty())
        return -1;

    auto it = m_webPageMap.find(url);
    if (it != m_webPageMap.end())
        return *it;

    auto iconQuery = m_database.prepare(R"(SELECT FaviconID FROM FaviconMap WHERE PageURL LIKE ?)");

    QString searchTemplate(QStringLiteral("%%1%"));
    std::array<QString, 2> searchTerms = { searchTemplate.arg(url.host()),
                                           searchTemplate.arg(URL(url).getSecondLevelDomain()) };
    for (size_t i = 0; i < searchTerms.size(); ++i)
    {
        iconQuery << searchTerms.at(i);

        if (iconQuery.next())
        {
            int iconId = 0;
            iconQuery >> iconId;
            return iconId;
        }

        iconQuery.reset();
    }

    return -1;
}

int FaviconStore::getFaviconIdForIconUrl(const QUrl &url)
{
    for (const auto &it : m_originMap)
    {
        if (CommonUtil::doUrlsMatch(url, it.second, true))
            return it.first;
        if (url.matches(it.second, QUrl::RemoveScheme | QUrl::RemoveQuery | QUrl::RemoveFragment))
            return it.first;
    }

    auto idQuery = m_database.prepare(R"(SELECT FaviconID FROM Favicons WHERE URL = ?)");
    idQuery << url;
    if (idQuery.next())
    {
        int iconId = 0;
        idQuery >> iconId;
        return iconId;
    }

    int id = m_newFaviconID++;
    sqlite::PreparedStatement &insertStmt = m_queryMap.at(StoredQuery::InsertFavicon);
    insertStmt.reset();
    insertStmt << id
               << url;

    if (!insertStmt.execute())
        qWarning() << "In FaviconStore::getFaviconIdForIconUrl - could not add favicon metadata to Favicons table.";

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

    sqlite::PreparedStatement &stmt = m_queryMap.at(StoredQuery::InsertIconData);
    stmt.reset();
    stmt << iconData;
    if (!stmt.execute())
        qWarning() << "In FaviconStore::getDataRecord - could not add favicon icon data to FaviconData table";

    auto result = m_iconDataMap.emplace(std::make_pair(faviconId, iconData));
    return result.first->second;
}

void FaviconStore::saveDataRecord(FaviconData &dataRecord)
{
    auto stmt = m_database.prepare(R"(UPDATE FaviconData SET Data = ? WHERE DataID = ?)");
    stmt << dataRecord.iconData
         << dataRecord.id;
    if (!stmt.execute())
        qWarning() << "In FaviconStore::saveDataRecord - could not update favicon data.";
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

    auto stmt = m_database.prepare(R"(INSERT OR REPLACE INTO FaviconMap(PageURL, FaviconID) VALUES (?, ?))");
    stmt << webPageUrl
         << faviconId;

    if (!stmt.execute())
        qDebug() << "In FaviconStore::addPageMapping - could not update webpage mapping.";
}

void FaviconStore::setupQueries()
{
    m_queryMap.clear();

    m_queryMap.insert(
                std::make_pair(StoredQuery::InsertFavicon,
                               m_database.prepare(R"(INSERT OR REPLACE INTO Favicons(FaviconID, URL) VALUES (?, ?))")));
    m_queryMap.insert(
                std::make_pair(StoredQuery::InsertIconData,
                               m_database.prepare(R"(INSERT OR REPLACE INTO FaviconData(DataID, FaviconID, Data) VALUES (?, ?, ?))")));
    m_queryMap.insert(
                std::make_pair(StoredQuery::FindIconExactURL,
                               m_database.prepare(R"(SELECT URL FROM Favicons WHERE FaviconID = (SELECT m.FaviconID FROM FaviconMap m WHERE m.PageURL = ?))")));
    m_queryMap.insert(
                std::make_pair(StoredQuery::FindIconLikeURL,
                               m_database.prepare(R"(SELECT URL FROM Favicons WHERE FaviconID IN (SELECT m.FaviconID FROM FaviconMap m WHERE m.PageURL LIKE ?))")));
}

bool FaviconStore::hasProperStructure()
{
    return hasTable(QStringLiteral("Favicons"))
            && hasTable(QStringLiteral("FaviconData"))
            && hasTable(QStringLiteral("FaviconMap"));
}

void FaviconStore::setup()
{   
    // Setup table structures    
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS Favicons(FaviconID INTEGER PRIMARY KEY, URL TEXT UNIQUE)"));
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS FaviconData(DataID INTEGER PRIMARY KEY, FaviconID INTEGER NOT NULL, Data BLOB, "
               "FOREIGN KEY(FaviconID) REFERENCES Favicons(FaviconID))"));
    exec(QStringLiteral("CREATE TABLE IF NOT EXISTS FaviconMap(MapID INTEGER PRIMARY KEY, PageURL TEXT UNIQUE, FaviconID INTEGER NOT NULL, "
               "FOREIGN KEY(FaviconID) REFERENCES Favicons(FaviconID))"));

    // Create indices
    exec(QStringLiteral("CREATE INDEX IF NOT EXISTS favicons_url ON Favicons(URL)"));
    exec(QStringLiteral("CREATE INDEX IF NOT EXISTS favicon_data_data_id ON FaviconData(DataID)"));
    exec(QStringLiteral("CREATE INDEX IF NOT EXISTS favicon_data_foreign_id ON FaviconData(FaviconID)"));
    exec(QStringLiteral("CREATE INDEX IF NOT EXISTS favicon_map_url ON FaviconMap(PageURL)"));
    exec(QStringLiteral("CREATE INDEX IF NOT EXISTS favicon_map_data_id ON FaviconMap(FaviconID)"));
}

void FaviconStore::load()
{
    setupQueries();

    auto query = m_database.prepare(R"(SELECT FaviconID, URL FROM Favicons)");
    while (query.next())
    {
        int iconId = 0;
        QUrl iconUrl;
        query >> iconId
              >> iconUrl;
        m_originMap.emplace(std::make_pair(iconId, iconUrl));
    }

    query = m_database.prepare(R"(SELECT DataID, FaviconID, Data FROM FaviconData)");
    while (query.next())
    {
        FaviconData data;
        query >> data;

        m_iconDataMap.emplace(std::make_pair(data.faviconId, data));
    }

    query = m_database.prepare(R"(SELECT PageURL, FaviconID FROM FaviconMap)");
    while (query.next())
    {
        QUrl pageUrl;
        int faviconId = 0;

        query >> pageUrl
              >> faviconId;

        m_webPageMap.insert(pageUrl, faviconId);
    }

    // Fetch maximum favicon ID and data ID values so new entry IDs can be calculated with more ease
    query = m_database.prepare(R"(SELECT MAX(FaviconID) FROM Favicons)");
    if (query.next())
    {
        query >> m_newFaviconID;
        m_newFaviconID++;
    }
    query = m_database.prepare(R"(SELECT MAX(DataID) FROM FaviconData)");
    if (query.next())
    {
        query >> m_newDataID;
        m_newDataID++;
    }
}
