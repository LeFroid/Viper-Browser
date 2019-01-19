#include "BrowserApplication.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"
#include "WebPageThumbnailStore.h"

#include <QByteArray>
#include <QBuffer>
#include <QVariantMap>

WebChannelPageInformation::WebChannelPageInformation(const WebPageInformation &pageInfo) :
    title(pageInfo.Title),
    url(pageInfo.URL),
    thumbnail(QLatin1String("data:image/png;base64, "))
{
    QByteArray data;
    QBuffer buffer(&data);
    pageInfo.Thumbnail.save(&buffer, "PNG");

    if (!data.isEmpty())
        thumbnail.append(data.toBase64());
    else
        thumbnail = QString();
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
        WebChannelPageInformation channelItem(pageInfo);

        QVariantMap item;
        item[QLatin1String("title")] = channelItem.title;
        item[QLatin1String("url")] = channelItem.url;
        item[QLatin1String("thumbnail")] = channelItem.thumbnail;
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
