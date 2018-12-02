#include "BrowserApplication.h"
#include "CommonUtil.h"
#include "FaviconStore.h"
#include "NetworkAccessManager.h"
#include "URL.h"

#include <array>
#include <stdexcept>
#include <QBuffer>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#include <QSqlError>
#include <QSqlRecord>
#include <QSvgRenderer>
#include <QDebug>

FaviconStore::FaviconStore(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("Favicons")),
    m_accessMgr(nullptr),
    m_reply(nullptr),
    m_favicons(),
    m_newFaviconID(1),
    m_newDataID(1),
    m_queryMap(),
    m_iconCache(25),
    m_mutex()
{
    setupQueries();
}

FaviconStore::~FaviconStore()
{
    save();
}

QIcon FaviconStore::getFavicon(const QUrl &url, bool useCache)
{
    std::lock_guard<std::mutex> _(m_mutex);

    QString pageUrl = getUrlAsString(url);
    if (pageUrl.isEmpty())
        return QIcon();

    const std::string urlStdStr = pageUrl.toStdString();

    if (useCache)
    {
        try
        {
            // Check for cache hit
            if (m_iconCache.has(urlStdStr))
                return m_iconCache.get(urlStdStr);
        }
        catch (std::out_of_range &err)
        {
            qDebug() << "FaviconStore::getFavicon - caught error while fetching icon from cache. Error: " << err.what();
        }
    }

    // Three step icon fetch process:
    // First, search DB for exact URL match.
    QSqlQuery *query = m_queryMap.at(StoredQuery::FindIconExactURL).get();
    query->bindValue(QLatin1String(":url"), pageUrl);
    if (query->exec() && query->first())
    {
        QString iconURL = query->value(0).toString();
        auto it = m_favicons.find(iconURL);
        if (it != m_favicons.end())
        {
            if (useCache)
                m_iconCache.put(urlStdStr, it->icon);
            return it->icon;
        }
    }

    // Second, search DB for host match. If still no hit, check for second level domain match
    QString searchTemplate("%%1%");
    std::array<QString, 2> fallbacks = { searchTemplate.arg(url.host()),
                                         searchTemplate.arg(URL(url).getSecondLevelDomain()) };

    query = m_queryMap.at(StoredQuery::FindIconLikeURL).get();
    for (size_t i = 0; i < fallbacks.size(); ++i)
    {
        query->bindValue(QLatin1String(":url"), fallbacks.at(i));
        if (query->exec())
        {
            while (query->next())
            {
                QString iconURL = query->value(0).toString();
                auto it = m_favicons.find(iconURL);
                if (it != m_favicons.end())
                {
                    if (useCache)
                        m_iconCache.put(urlStdStr, it->icon);
                    return it->icon;
                }
            }
        }
    }

    return QIcon(QLatin1String(":/blank_favicon.png"));
}

void FaviconStore::updateIcon(const QString &iconHRef, const QUrl &pageUrl, QIcon pageIcon)
{
    std::lock_guard<std::mutex> _(m_mutex);

    if (iconHRef.isEmpty() || iconHRef.startsWith(QLatin1String("data")))
        return;

    QString pageUrlStr = getUrlAsString(pageUrl);

    auto it = m_favicons.find(iconHRef);
    if (it != m_favicons.end())
    {
        it->urlSet.insert(pageUrlStr);

        if (!pageIcon.isNull())
        {
            it->icon = pageIcon;
            saveToDB(iconHRef, *it);
        }
    }
    else
    {
        // Fetch icon and add info to hash map
        FaviconInfo info;
        info.iconID = m_newFaviconID++;
        info.dataID = m_newDataID++;
        info.icon = pageIcon;
        info.urlSet.insert(pageUrlStr);
        m_favicons.insert(iconHRef, info);

        // Manually make request for icon if QWebSettings could not get the icon
        if (!info.icon.isNull())
        {
            // Save to DB before returning
            saveToDB(iconHRef, info);
            return;
        }

        if (!m_accessMgr)
            m_accessMgr = BrowserApplication::instance()->getNetworkAccessManager();

        QNetworkRequest request(QUrl::fromUserInput(iconHRef));
        m_reply = m_accessMgr->get(request);
        if (m_reply->isFinished())
            onReplyFinished();
        else
            connect(m_reply, &QNetworkReply::finished, this, &FaviconStore::onReplyFinished);
    }
}

void FaviconStore::onReplyFinished()
{
    std::lock_guard<std::mutex> _(m_mutex);

    if (!m_reply)
        return;

    // Update icon in hash map
    auto it = m_favicons.find(m_reply->url().toString());
    if (it != m_favicons.end())
    {
        QString format = QFileInfo(getUrlAsString(m_reply->url())).suffix();
        QByteArray data = m_reply->readAll();
        if (data.isNull())
        {
            m_favicons.erase(it);
            return;
        }

        QImage img;
        bool success = false;

        // Handle compressed data
        if (format.compare(QLatin1String("gzip")) == 0)
        {
            data = qUncompress(data);
            format.clear();
        }
        // Handle SVG favicons
        else if (format.compare(QLatin1String("svg")) == 0)
        {
            QSvgRenderer svgRenderer(data);
            img = QImage(32, 32, QImage::Format_ARGB32);
            QPainter painter(&img);
            svgRenderer.render(&painter);
            success = !img.isNull();
        }
        // Default handler
        else
        {
            QBuffer buffer(&data);
            const char *imgFormat = format.isEmpty() ? nullptr : format.toStdString().c_str();
            success = img.load(&buffer, imgFormat);
        }

        if (!success)
        {
            qDebug() << "FaviconStore::onReplyFinished - failed to load image from response. Format was " << format;
            m_favicons.erase(it);
        }
        else
        {
            it->icon = QIcon(QPixmap::fromImage(img));

            // Insert favicon data into the DB
            saveToDB(it.key(), it.value());
        }
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

QString FaviconStore::getUrlAsString(const QUrl &url) const
{
    return url.toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment);
}

void FaviconStore::saveToDB(const QString &faviconUrl, const FaviconInfo &favicon)
{
    // ignore when the location itself is a data blob
    if (faviconUrl.startsWith(QLatin1String("data:")))
        return;

    QSqlQuery *query = m_queryMap.at(StoredQuery::InsertFavicon).get();
    query->bindValue(QLatin1String(":iconId"), favicon.iconID);
    query->bindValue(QLatin1String(":url"), faviconUrl);
    if (!query->exec())
        qDebug() << "In FaviconStore::saveToDB - could not add favicon metadata to Favicons table. Message: "
                 << query->lastError().text();

    query = m_queryMap.at(StoredQuery::InsertIconData).get();
    query->bindValue(QLatin1String(":dataId"), favicon.dataID);
    query->bindValue(QLatin1String(":iconId"), favicon.iconID);
    query->bindValue(QLatin1String(":data"), CommonUtil::iconToBase64(favicon.icon));
    if (!query->exec())
        qDebug() << "In FaviconStore::saveToDB - could not add favicon icon data to FaviconData table. Message: "
                 << query->lastError().text();

    //emit faviconAdded(favicon);
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
    savedQuery->prepare(QLatin1String("SELECT URL FROM Favicons WHERE FaviconID = (SELECT m.FaviconID FROM FaviconMap m WHERE m.PageURL LIKE (:url))"));
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

    setupQueries();
}

void FaviconStore::load()
{
    QSqlQuery query(m_database);
    if (query.exec(QLatin1String("SELECT f.FaviconID, f.URL, d.DataID, d.Data FROM Favicons f INNER JOIN FaviconData d ON f.FaviconID = d.FaviconID")))
    {
        QSqlRecord rec = query.record();
        int idFaviconID = rec.indexOf(QLatin1String("FaviconID"));
        int idDataID = rec.indexOf(QLatin1String("DataID"));
        int idUrl = rec.indexOf(QLatin1String("URL"));
        int idData = rec.indexOf(QLatin1String("Data"));
        while (query.next())
        {
            QString iconUrl = query.value(idUrl).toString();
            FaviconInfo info;
            info.iconID = query.value(idFaviconID).toInt();
            info.dataID = query.value(idDataID).toInt();
            info.icon = CommonUtil::iconFromBase64(query.value(idData).toByteArray());
            m_favicons.insert(iconUrl, info);
        }
    }
    else
        qDebug() << "[Error]: In FaviconStore::load() - Unable to fetch favicon data. Message: " << query.lastError().text();

    // Fetch maximum favicon ID and data ID values so new entry IDs can be calculated with more ease
    if (query.exec(QLatin1String("SELECT MAX(FaviconID) FROM Favicons")) && query.first())
        m_newFaviconID = query.value(0).toInt() + 1;
    if (query.exec(QLatin1String("SELECT MAX(DataID) FROM FaviconData")) && query.first())
        m_newDataID = query.value(0).toInt() + 1;
}

void FaviconStore::save()
{
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
        for (const auto page : it->urlSet)
        {
            queryIconMap.bindValue(QLatin1String(":pageUrl"), page);
            queryIconMap.bindValue(QLatin1String(":iconId"), iconID);
            if (!queryIconMap.exec())
                qDebug() << "[Error]: In FaviconStore::save() - Could not map URL to favicon in database. Message: "
                         << queryIconMap.lastError().text();
        }
    }
}
