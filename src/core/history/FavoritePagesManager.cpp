#include "BrowserApplication.h"
#include "CommonUtil.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"
#include "WebPageThumbnailStore.h"

#include <chrono>
#include <QByteArray>
#include <QBuffer>
#include <QFile>
#include <QSet>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>

const QString FavoritePagesManager::Version = QStringLiteral("1.1");

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

FavoritePagesManager::FavoritePagesManager(HistoryManager *historyMgr, WebPageThumbnailStore *thumbnailStore, const QString &dataFile, QObject *parent) :
    QObject(parent),
    m_timerId(0),
    m_dataFileName(dataFile),
    m_excludedPages(),
    m_favoritePages(),
    m_mostVisitedPages(),
    m_historyManager(historyMgr),
    m_thumbnailStore(thumbnailStore)
{
    setObjectName(QLatin1String("favoritePageManager"));

    loadFavorites();
    loadFromHistory();

    // update history-based list every 10 minutes
    using namespace std::chrono_literals;
    m_timerId = startTimer(10min);
}

FavoritePagesManager::~FavoritePagesManager()
{
    killTimer(m_timerId);
    save();
}

bool FavoritePagesManager::isPresent(const QUrl &url) const
{
    for (const auto &pageInfo : m_favoritePages)
    {
        if (pageInfo.URL == url)
            return true;
    }

    for (const auto &pageInfo : m_mostVisitedPages)
    {
        if (pageInfo.URL == url)
            return true;
    }

    return false;
}

QVariantList FavoritePagesManager::getFavorites() const
{
    QVariantList result;

    auto addPagesToResult = [&](const std::vector<WebPageInformation> &pageContainer) {
        for (const auto &pageInfo : pageContainer)
        {
            QVariantMap item;
            item[QLatin1String("position")] = pageInfo.Position;
            item[QLatin1String("title")] = pageInfo.Title;
            item[QLatin1String("url")] = pageInfo.URL;
            item[QLatin1String("thumbnail")] = pageInfo.getThumbnailInBase64();
            result.append(item);
        }
    };

    addPagesToResult(m_favoritePages);
    addPagesToResult(m_mostVisitedPages);

    return result;
}

void FavoritePagesManager::addFavorite(const QUrl &url, const QString &title)
{
    if (!m_historyManager
            || !m_thumbnailStore
            || isPresent(url))
        return;

    WebPageInformation pageInfo;
    pageInfo.Position = static_cast<int>(m_favoritePages.size());
    pageInfo.URL = url;
    pageInfo.Title = title;
    pageInfo.Thumbnail = m_thumbnailStore->getThumbnail(url);

    if (title.isEmpty())
    {
        HistoryEntry record = m_historyManager->getEntry(url);
        pageInfo.Title = record.Title;
    }

    m_favoritePages.push_back(pageInfo);
}

void FavoritePagesManager::removeEntry(const QUrl &url)
{
    if (!isUrlHidden(url))
    {
        // Add and then automatically remove entry from exclusion list after 3 weeks
        NewTabHiddenEntry entry { url, QDateTime::currentDateTime().addDays(21) };
        m_excludedPages.push_back(entry);
    }

    auto removeFromContainer = [&](std::vector<WebPageInformation> &pageContainer) {
        for (auto it = pageContainer.begin(); it != pageContainer.end();)
        {
            if (it->URL == url)
                it = pageContainer.erase(it);
            else
                ++it;
        }
    };

    removeFromContainer(m_favoritePages);
    removeFromContainer(m_mostVisitedPages);
}

void FavoritePagesManager::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timerId)
        loadFromHistory();
    else
        QObject::timerEvent(event);
}

bool FavoritePagesManager::isUrlHidden(const QUrl &url)
{
    auto it = std::find_if(m_excludedPages.begin(), m_excludedPages.end(), [url](const NewTabHiddenEntry &entry) {
        return CommonUtil::doUrlsMatch(entry.URL, url, false);
    });

    return it != m_excludedPages.end();
}

void FavoritePagesManager::loadFavorites()
{
    if (!m_thumbnailStore)
        return;

    QFile dataFile(m_dataFileName);
    if (!dataFile.exists() || !dataFile.open(QIODevice::ReadOnly))
        return;

    QByteArray pageData = dataFile.readAll();
    dataFile.close();

    // Parse data file as a JSON formatted document
    QJsonDocument doc(QJsonDocument::fromJson(pageData));
    if (!doc.isObject())
        return;

    QJsonObject rootObject = doc.object();

    const QString savedVersion = rootObject.value(QLatin1String("version")).toString();

    QSet<QUrl> favoritedUrls;

    // Load from the two arrays: the favorite page array, and the excluded pages array
    QJsonArray favoriteArray = rootObject.value(QLatin1String("favorites")).toArray();
    for (auto it = favoriteArray.begin(); it != favoriteArray.end(); ++it)
    {
        QJsonObject currentPage = it->toObject();

        WebPageInformation pageInfo;
        pageInfo.Position = currentPage.value(QLatin1String("position")).toInt();
        pageInfo.Title = currentPage.value(QLatin1String("title")).toString();
        pageInfo.URL = QUrl(currentPage.value(QLatin1String("url")).toString());
        pageInfo.Thumbnail = m_thumbnailStore->getThumbnail(pageInfo.URL);

        m_favoritePages.push_back(pageInfo);
        favoritedUrls.insert(pageInfo.URL);
    }

    QJsonArray excludedArray = rootObject.value(QLatin1String("excludes")).toArray();
    const bool arrayWithUrlsOnly = savedVersion.compare(QLatin1String("1.0")) == 0;
    const QDateTime currentTime = QDateTime::currentDateTime();
    for (auto it = excludedArray.begin(); it != excludedArray.end(); ++it)
    {
        if (arrayWithUrlsOnly)
        {
            QUrl url = QUrl(it->toString());
            if (!favoritedUrls.contains(url))
            {
                NewTabHiddenEntry entry { url, QDateTime::currentDateTime().addDays(21) };
                m_excludedPages.push_back(entry);
            }
        }
        else
        {
            QJsonObject pageInfo = it->toObject();

            NewTabHiddenEntry entry;
            entry.URL = QUrl(pageInfo.value(QLatin1String("url")).toString());

            if (favoritedUrls.contains(entry.URL))
                continue;

            entry.ExpiresOn = QDateTime::fromString(
                                pageInfo.value(QLatin1String("expiresOn")).toString(),
                                Qt::ISODate);

            if (entry.ExpiresOn > currentTime)
                m_excludedPages.push_back(entry);
        }
    }
}

void FavoritePagesManager::loadFromHistory()
{
    if (!m_historyManager)
        return;

    // Load most frequent visits, and then remove any that the user requested to be excluded from
    // the new tab page
    const int numResults = 10 + static_cast<int>(m_excludedPages.size());
    m_historyManager->loadMostVisitedEntries(numResults, [=](std::vector<WebPageInformation> &&results){
        int itemPosition = static_cast<int>(m_favoritePages.size());
        m_mostVisitedPages = std::move(results);
        for (auto it = m_mostVisitedPages.begin(); it != m_mostVisitedPages.end();)
        {
            if (isUrlHidden(it->URL))
                it = m_mostVisitedPages.erase(it);
            else
            {
                // Set position and get the thumbnail if we will keep this result
                it->Position = itemPosition++;
                it->Thumbnail = m_thumbnailStore->getThumbnail(it->URL);
                ++it;
            }
        }
    });
}

void FavoritePagesManager::save()
{
    QFile saveFile(m_dataFileName);
    if (!saveFile.open(QIODevice::WriteOnly))
        return;

    QJsonObject rootObject;

    rootObject.insert(QLatin1String("version"), QJsonValue(Version));

    // Save array of favorited pages
    QJsonArray favoriteArray;
    for (const WebPageInformation &pageInfo : m_favoritePages)
    {
        QJsonObject currentPage;
        currentPage.insert(QLatin1String("position"), pageInfo.Position);
        currentPage.insert(QLatin1String("url"), pageInfo.URL.toString());
        currentPage.insert(QLatin1String("title"), pageInfo.Title);

        favoriteArray.append(QJsonValue(currentPage));
    }
    rootObject.insert(QLatin1String("favorites"), QJsonValue(favoriteArray));

    // Save array of excluded pages
    QJsonArray excludedArray;
    const QDateTime currentTime = QDateTime::currentDateTime();
    for (const NewTabHiddenEntry &entry : m_excludedPages)
    {
        if (currentTime >= entry.ExpiresOn)
            continue;
        QJsonObject hiddenPage;
        hiddenPage.insert(QLatin1String("url"), entry.URL.toString());
        hiddenPage.insert(QLatin1String("expiresOn"), entry.ExpiresOn.toString(Qt::ISODate));

        excludedArray.append(QJsonValue(hiddenPage));
    }
    rootObject.insert(QLatin1String("excludes"), QJsonValue(excludedArray));

    // Format root object as a document, and write to the file
    QJsonDocument doc(rootObject);
    saveFile.write(doc.toJson());
    saveFile.close();
}
