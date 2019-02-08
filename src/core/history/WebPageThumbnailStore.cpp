#include "BookmarkNode.h"
#include "BookmarkManager.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"
#include "WebPageThumbnailStore.h"
#include "WebView.h"
#include "WebWidget.h"

#include <algorithm>
#include <array>
#include <set>
#include <QBuffer>
#include <QMimeType>
#include <QPixmap>
#include <QPointer>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QTimer>

#include <QDebug>

WebPageThumbnailStore::WebPageThumbnailStore(ViperServiceLocator &serviceLocator, const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("ThumbnailDB")),
    m_thumbnails(),
    m_bookmarkManager(serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager")),
    m_historyManager(serviceLocator.getServiceAs<HistoryManager>("HistoryManager")),
    m_mimeDatabase()
{
}

WebPageThumbnailStore::~WebPageThumbnailStore()
{
    save();
}

QImage WebPageThumbnailStore::getThumbnail(const QUrl &url)
{
    const QString host = url.host().toLower();
    if (host.isEmpty())
        return QImage();

    // First, check in-memory storage. Then check the database for a thumbnail.
    auto it = m_thumbnails.find(host);
    if (it != m_thumbnails.end())
        return it.value();

    QSqlQuery query(m_database);
    query.prepare(QLatin1String("SELECT Thumbnail FROM Thumbnails WHERE Host = (:host)"));
    query.bindValue(QLatin1String(":host"), host);
    if (query.exec() && query.first())
    {
        QByteArray data = query.value(0).toByteArray();
        QByteArray decoded = QByteArray::fromBase64(data);

        QBuffer buffer(&decoded);
        QImage image;
        image.load(&buffer, "PNG");

        m_thumbnails.insert(host, image);

        return image;
    }

    return QImage();
}

void WebPageThumbnailStore::onPageLoaded(bool ok)
{
    if (!ok)
        return;

    QPointer<WebWidget> ww = qobject_cast<WebWidget*>(sender());
    if (ww.isNull() || ww->isOnBlankPage() || ww->isHibernating())
        return;

    // Check if we should ignore this page
    const QUrl url = ww->url();
    const QUrl originalUrl = ww->getOriginalUrl();
    const QString scheme = url.scheme().toLower();

    const std::array<QString, 2> ignoreSchemes { QLatin1String("qrc"), QLatin1String("viper") };
    if (std::find(ignoreSchemes.begin(), ignoreSchemes.end(), scheme) != ignoreSchemes.end())
        return;

    if (url.isEmpty() || originalUrl.isEmpty() || scheme.isEmpty())
        return;

    const QString fileName = url.fileName();
    if (!fileName.isEmpty())
    {
        const QString mimeType = m_mimeDatabase.mimeTypeForFile(fileName).name();
        if (mimeType.startsWith(QLatin1String("image")) || mimeType.startsWith(QLatin1String("video")))
            return;
    }

    std::vector<QUrl> urls { originalUrl };
    if (originalUrl.adjusted(QUrl::RemoveScheme) != url.adjusted(QUrl::RemoveScheme))
        urls.push_back(url);

    // Wait one second before trying to get the thumbnails, otherwise
    // we might get a blank thumbnail
    QTimer::singleShot(1000, this, [this, ww, urls]() {
        if (ww.isNull())
            return;
        if (WebView *view = ww->view())
        {
            if (view->getProgress() < 100)
                return;

            const QPixmap &pixmap = view->getThumbnail();
            if (pixmap.isNull())
                return;

            for (const QUrl &url : urls)
            {
                const QString host = url.host().toLower();
                if (!host.isEmpty())
                    m_thumbnails.insert(host, pixmap.toImage());
            }
        }
    });
}

bool WebPageThumbnailStore::hasProperStructure()
{
    return hasTable(QLatin1String("Thumbnails"));
}

void WebPageThumbnailStore::setup()
{
    // Thumbnails table:
    // Id  |  Host  | Thumbnail
    // pk    string   string/blob

    // Setup table structure
    QSqlQuery query(m_database);
    if (!query.exec(QLatin1String("CREATE TABLE IF NOT EXISTS Thumbnails(Id INTEGER PRIMARY KEY, Host TEXT UNIQUE, Thumbnail BLOB)")))
        qWarning() << "WebPageThumbnailStore - could not create thumbnail database. Error message: "
                   << query.lastError().text();
}

void WebPageThumbnailStore::load()
{
    // load only when needed, not during instantiation
}

void WebPageThumbnailStore::save()
{
    // iterate through the in-memory collection, and if any of the hostnames of a page's thumbnail
    // is (1) in the top 100 most visited web pages (see HistoryManager), or (2) is favorited by
    // the user, or (3) is bookmarked, then save to the DB
    if (!m_historyManager || !m_bookmarkManager)
        return;

    std::set<std::string> mostVisitedHosts;

    // Load top history entries into set
    int historyLimit = std::min(m_thumbnails.size(), 100);
    auto mostVisitedHistoryPages = m_historyManager->loadMostVisitedEntries(historyLimit);

    for (const auto &entry : mostVisitedHistoryPages)
    {
        const std::string host = entry.URL.host().toLower().toStdString();
        if (!host.empty())
            mostVisitedHosts.insert(host);
    }

    // Load all bookmarks into set
    for (auto it : *m_bookmarkManager)
    {
        if (it->getType() == BookmarkNode::Bookmark)
        {
            const std::string host = it->getURL().host().toLower().toStdString();
            if (!host.empty())
                mostVisitedHosts.insert(host);
        }
    }

    // Save applicable thumbnails
    QSqlQuery query(m_database);
    query.prepare(QLatin1String("INSERT OR REPLACE INTO Thumbnails(Host, Thumbnail) VALUES(:host, :thumbnail)"));

    for (auto it = m_thumbnails.begin(); it != m_thumbnails.end(); ++it)
    {
        const std::string host = it.key().toStdString();
        if (mostVisitedHosts.find(host) == mostVisitedHosts.end())
            continue;

        const QImage &image = it.value();
        if (image.isNull())
            continue;

        QByteArray data;
        QBuffer buffer(&data);
        image.save(&buffer, "PNG");

        query.bindValue(QLatin1String(":host"), it.key());
        query.bindValue(QLatin1String(":thumbnail"), data.toBase64());
        if (!query.exec())
            qWarning() << "WebPageThumbnailStore - could not save thumbnail to database. Message: "
                       << query.lastError().text();
    }
}
