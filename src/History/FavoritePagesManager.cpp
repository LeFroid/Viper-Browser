#include "BrowserApplication.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"
#include "WebPageThumbnailStore.h"

#include <QByteArray>
#include <QBuffer>
#include <QVariantMap>

QString WebPageInformation::getThumbnailInBase64() const
{
    QString result = QLatin1String("data:image/png;base64, ");

    QByteArray data;
    QBuffer buffer(&data);
    Thumbnail.save(&buffer, "PNG");

    if (data.isEmpty())
        return QString();

    result.append(data.toBase64());
    return result;
}

FavoritePagesManager::FavoritePagesManager(HistoryManager *historyMgr, WebPageThumbnailStore *thumbnailStore, QObject *parent) :
    QObject(parent),
    m_timerId(0),
    m_mostVisitedPages(),
    m_historyManager(historyMgr),
    m_thumbnailStore(thumbnailStore)
{
    setObjectName(QLatin1String("favoritePageManager"));

    // update page list every 10 minutes
    m_timerId = startTimer(1000 * 60 * 10);
    loadFavorites();
}

FavoritePagesManager::~FavoritePagesManager()
{
    killTimer(m_timerId);
}

QVariantList FavoritePagesManager::getFavorites() const
{
    QVariantList result;
    for (const auto &pageInfo : m_mostVisitedPages)
    {
        QVariantMap item;
        item[QLatin1String("title")] = pageInfo.Title;
        item[QLatin1String("url")] = pageInfo.URL;
        item[QLatin1String("thumbnail")] = pageInfo.getThumbnailInBase64();
        result.append(item);
    }

    return result;
}

void FavoritePagesManager::timerEvent(QTimerEvent */*event*/)
{
    loadFavorites();
}

void FavoritePagesManager::loadFavorites()
{
    if (!m_historyManager)
        return;

    m_mostVisitedPages = m_historyManager->loadMostVisitedEntries(10);

    for (auto &pageInfo : m_mostVisitedPages)
    {
        pageInfo.Thumbnail = m_thumbnailStore->getThumbnail(pageInfo.URL);
    }
}
