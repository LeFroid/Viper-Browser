#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include "NetworkAccessManager.h"

#include <QBuffer>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QWebSettings>
#include <QDebug>

FaviconStorage::FaviconStorage(bool firstRun, const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, "Favicons"),
    m_accessMgr(nullptr),
    m_reply(nullptr),
    m_favicons(),
    m_newFaviconID(0),
    m_newDataID(0),
    m_queryInsertFavicon(nullptr),
    m_queryInsertIconData(nullptr)
{
    if (firstRun)
        setup();

    load();

    m_queryInsertFavicon = new QSqlQuery(m_database);
    m_queryInsertFavicon->prepare("INSERT OR REPLACE INTO Favicons(FaviconID, URL) VALUES(:iconId, :url)");

    m_queryInsertIconData = new QSqlQuery(m_database);
    m_queryInsertIconData->prepare("INSERT OR REPLACE INTO FaviconData(DataID, FaviconID, Data) VALUES(:dataId, :iconId, :data)");
}

FaviconStorage::~FaviconStorage()
{
    delete m_queryInsertFavicon;
    delete m_queryInsertIconData;

    save();
}

QIcon FaviconStorage::getFavicon(const QString &url) const
{
    QSqlQuery query(m_database);
    query.prepare("SELECT URL FROM Favicons WHERE FaviconID = (SELECT m.FaviconID FROM FaviconMap m WHERE m.PageURL = (:url))");
    query.bindValue(":url", url);
    if (query.exec() && query.first())
    {
        QString iconURL = query.value(0).toString();
        auto it = m_favicons.find(iconURL);
        if (it != m_favicons.end())
            return it->icon;
    }
    return QIcon();
}

void FaviconStorage::updateIcon(const QString &iconHRef, const QString &pageUrl)
{
    auto it = m_favicons.find(iconHRef);
    if (it != m_favicons.end())
        it->urlSet.insert(pageUrl);
    else
    {
        // Fetch icon and add info to hash map
        FaviconInfo info;
        info.iconID = m_newFaviconID++;
        info.dataID = m_newDataID++;
        info.icon = QWebSettings::iconForUrl(pageUrl);
        info.urlSet.insert(pageUrl);
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
            connect(m_reply, &QNetworkReply::finished, this, &FaviconStorage::onReplyFinished);
    }
}

void FaviconStorage::onReplyFinished()
{
    if (!m_reply)
        return;

    // Update icon in hash map
    auto it = m_favicons.find(m_reply->url().toString());
    if (it != m_favicons.end())
    {
        qDebug() << "FaviconStorage::onReplyFinished - found structure in hash map to update icon with";
        QByteArray data = m_reply->readAll();
        QBuffer buffer(&data);
        QImage img;
        QString format = QFileInfo(m_reply->url().toString()).suffix();
        if (!img.load(&buffer, format.toStdString().c_str()))
        {
            qDebug() << "FaviconStorage::onReplyFinished - failed to load image from response. Format was " << format;
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

QIcon FaviconStorage::iconFromBase64(QByteArray data)
{
    QByteArray decoded = QByteArray::fromBase64(data);

    QBuffer buffer(&decoded);
    QImage img;
    img.load(&buffer, "PNG");

    return QIcon(QPixmap::fromImage(img));
}

QByteArray FaviconStorage::iconToBase64(QIcon icon)
{
    // First convert the icon into a QImage, and from that place data into a byte array
    QImage img = icon.pixmap(16, 16).toImage();

    QByteArray data;
    QBuffer buffer(&data);
    img.save(&buffer, "PNG");

    return data.toBase64();
}

void FaviconStorage::saveToDB(const QString &faviconUrl, const FaviconInfo &favicon)
{
    m_queryInsertFavicon->bindValue(":iconId", favicon.iconID);
    m_queryInsertFavicon->bindValue(":url", faviconUrl);
    if (!m_queryInsertFavicon->exec())
        qDebug() << "In FaviconStorage::saveToDB - could not add favicon metadata to Favicons table. Message: "
                 << m_queryInsertFavicon->lastError().text();

    m_queryInsertIconData->bindValue(":dataId", favicon.dataID);
    m_queryInsertIconData->bindValue(":iconId", favicon.iconID);
    m_queryInsertIconData->bindValue(":data", iconToBase64(favicon.icon));
    if (!m_queryInsertIconData->exec())
        qDebug() << "In FaviconStorage::saveToDB - could not add favicon icon data to FaviconData table. Message: "
                 << m_queryInsertIconData->lastError().text();
}

void FaviconStorage::setup()
{
    // Setup table structures
    QSqlQuery query(m_database);
    query.exec("CREATE TABLE IF NOT EXISTS Favicons(FaviconID INTEGER PRIMARY KEY, URL TEXT UNIQUE)");
    query.exec("CREATE TABLE IF NOT EXISTS FaviconData(DataID INTEGER PRIMARY KEY, FaviconID INTEGER NOT NULL, Data BLOB, "
               "FOREIGN KEY(FaviconID) REFERENCES Favicons(FaviconID))");
    query.exec("CREATE TABLE IF NOT EXISTS FaviconMap(MapID INTEGER PRIMARY KEY, PageURL TEXT UNIQUE, FaviconID INTEGER NOT NULL, "
               "FOREIGN KEY(FaviconID) REFERENCES Favicons(FaviconID))");
    // Create indices
    query.exec("CREATE INDEX favicons_url ON Favicons(URL)");
    query.exec("CREATE INDEX favicon_data_data_id ON FaviconData(DataID)");
    query.exec("CREATE INDEX favicon_data_foreign_id ON FaviconData(FaviconID)");
    query.exec("CREATE INDEX favicon_map_url ON FaviconMap(PageURL)");
    query.exec("CREATE INDEX favicon_map_data_id ON FaviconMap(FaviconID)");
}

void FaviconStorage::load()
{
    QSqlQuery query(m_database);
    if (query.exec("SELECT f.FaviconID, f.URL, d.DataID, d.Data FROM Favicons f INNER JOIN FaviconData d ON f.FaviconID = d.FaviconID"))
    {
        QSqlRecord rec = query.record();
        int idFaviconID = rec.indexOf("FaviconID");
        int idDataID = rec.indexOf("DataID");
        int idUrl = rec.indexOf("URL");
        int idData = rec.indexOf("Data");
        while (query.next())
        {
            QString iconUrl = query.value(idUrl).toString();
            FaviconInfo info;
            info.iconID = query.value(idFaviconID).toInt();
            info.dataID = query.value(idDataID).toInt();
            info.icon = iconFromBase64(query.value(idData).toByteArray());
            m_favicons.insert(iconUrl, info);
        }
    }
    else
        qDebug() << "[Error]: In FaviconStorage::load() - Unable to fetch favicon data. Message: " << query.lastError().text();

    // Fetch maximum favicon ID and data ID values so new entry IDs can be calculated with more ease
    if (query.exec("SELECT MAX(FaviconID) FROM Favicons") && query.first())
        m_newFaviconID = query.value(0).toInt() + 1;
    if (query.exec("SELECT MAX(DataID) FROM FaviconData") && query.first())
        m_newDataID = query.value(0).toInt() + 1;
}

void FaviconStorage::save()
{
    // Prepare query to save icon:url mapping
    QSqlQuery queryIconMap(m_database);
    queryIconMap.prepare("INSERT OR IGNORE INTO FaviconMap(PageURL, FaviconID) VALUES(:pageUrl, :iconId)");

    // Iterate through favicon entries
    int iconID;
    for (auto it = m_favicons.cbegin(); it != m_favicons.cend(); ++it)
    {
        if (it->icon.isNull())
            continue;

        iconID = it->iconID;
        for (const auto page : it->urlSet)
        {
            queryIconMap.bindValue(":pageUrl", page);
            queryIconMap.bindValue(":iconId", iconID);
            if (!queryIconMap.exec())
                qDebug() << "[Error]: In FaviconStorage::save() - Could not map URL to favicon in database. Message: "
                         << queryIconMap.lastError().text();
        }
    }
}
