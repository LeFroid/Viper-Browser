#include "WebPageThumbnailStore.h"
#include "WebView.h"
#include "WebWidget.h"

#include <array>
#include <QBuffer>
#include <QPixmap>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QTimer>

#include <QDebug>

WebPageThumbnailStore::WebPageThumbnailStore(const QString &databaseFile, QObject *parent) :
    QObject(parent),
    DatabaseWorker(databaseFile, QLatin1String("ThumbnailDB")),
    m_thumbnails()
{
}

WebPageThumbnailStore::~WebPageThumbnailStore()
{
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

        return image;
    }

    return QImage();
}

void WebPageThumbnailStore::onPageLoaded(bool ok)
{
    if (!ok)
        return;

    WebWidget *ww = qobject_cast<WebWidget*>(sender());
    if (ww == nullptr || ww->isOnBlankPage())
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

    std::vector<QUrl> urls { originalUrl };
    if (originalUrl.adjusted(QUrl::RemoveScheme) != url.adjusted(QUrl::RemoveScheme))
        urls.push_back(url);

    // Wait one second before trying to get the thumbnails, otherwise
    // we might get a blank thumbnail
    QTimer::singleShot(1000, this, [this, ww, urls](){
        if (WebView *view = ww->view())
        {
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
}
