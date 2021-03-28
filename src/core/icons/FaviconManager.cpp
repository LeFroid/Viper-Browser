#include "CommonUtil.h"
#include "DatabaseFactory.h"
#include "FaviconManager.h"
#include "NetworkAccessManager.h"
#include "URL.h"

#include <functional>
#include <stdexcept>

#include <QBuffer>
#include <QDebug>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QPainter>
#include <QSvgRenderer>
#include <QtConcurrent>

FaviconManager::FaviconManager(const QString &databaseFile) :
    QObject(nullptr),
    m_faviconStore(nullptr),
    m_networkAccessManager(nullptr),
    m_iconMap(),
    m_iconCache(64),
    m_mutex()
{
    setObjectName(QStringLiteral("FaviconManager"));
    m_faviconStore = DatabaseFactory::createWorker<FaviconStore>(databaseFile);
}

void FaviconManager::setNetworkAccessManager(NetworkAccessManager *networkAccessManager)
{
    m_networkAccessManager = networkAccessManager;
}

QIcon FaviconManager::getFavicon(const QUrl &url)
{
    QString pageUrl = getUrlAsString(url);
    if (!m_faviconStore || pageUrl.isEmpty())
        return QIcon(QStringLiteral(":/blank_favicon.png"));

    const std::string urlStdStr = pageUrl.toStdString();

    // Check for cache hit
    try
    {
        if (m_iconCache.has(urlStdStr))
        {
            return m_iconCache.get(urlStdStr);
        }
    }
    catch (std::out_of_range &err)
    {
        qDebug() << "FaviconManager::getFavicon - caught error while fetching icon from cache. Error: " << err.what();
    }

    int iconId = m_faviconStore->getFaviconId(url);
    if (iconId < 0)
        return QIcon(QStringLiteral(":/blank_favicon.png"));

    auto it = m_iconMap.find(iconId);
    if (it == m_iconMap.end())
    {
        auto &record = m_faviconStore->getDataRecord(iconId);
        if (record.iconData.isEmpty())
            return QIcon(QStringLiteral(":/blank_favicon.png"));

        QIcon icon = CommonUtil::iconFromBase64(record.iconData);
        auto result = m_iconMap.emplace(std::make_pair(iconId, icon));
        if (result.second)
            it = result.first;
        else
            return icon;
    }

    try
    {
        std::lock_guard<std::mutex> _(m_mutex);
        m_iconCache.put(urlStdStr, it->second);
    }
    catch (std::out_of_range &err)
    {
        qDebug() << "FaviconManager::getFavicon - caught error while updating icon cache. Error: " << err.what();
    }

    return it->second;
}

void FaviconManager::updateIcon(const QUrl &iconUrl, const QUrl &pageUrl, const QIcon &pageIcon)
{
    if (!m_faviconStore
            || iconUrl.isEmpty()
            || iconUrl.scheme().startsWith(QStringLiteral("data")))
        return;

    const QString pageUrlStr = getUrlAsString(pageUrl);
    const std::string urlStdStr = pageUrlStr.toStdString();

    QByteArray pageIconData = CommonUtil::iconToBase64(pageIcon);

    if (!pageIconData.isEmpty() && m_iconCache.has(urlStdStr))
    {
        try
        {
            std::lock_guard<std::mutex> _(m_mutex);
            m_iconCache.put(urlStdStr, pageIcon);
        }
        catch (std::out_of_range &err)
        {
            qDebug() << "FaviconManager::updateIcon - caught error while updating icon cache. Error: " << err.what();
        }
    }

    const int iconId = m_faviconStore->getFaviconIdForIconUrl(iconUrl);
    FaviconData &dataRecord = m_faviconStore->getDataRecord(iconId);
    if (dataRecord.iconData.isEmpty() || !pageIconData.isEmpty())
        dataRecord.iconData = pageIconData;

    // add page url -> icon mapping to favicon store
    m_faviconStore->addPageMapping(pageUrl, iconId);

    if (!dataRecord.iconData.isEmpty())
    {
        m_faviconStore->saveDataRecord(dataRecord);

        QIcon icon = CommonUtil::iconFromBase64(dataRecord.iconData);
        auto it = m_iconMap.find(iconId);
        if (it != m_iconMap.end())
            it->second = icon;
        else
            m_iconMap.emplace(std::make_pair(iconId, icon));

        return;
    }

    if (!m_networkAccessManager)
        return;

    QNetworkRequest request(iconUrl);
    QNetworkReply *reply = m_networkAccessManager->get(request);
    if (reply->isFinished())
        onReplyFinished(reply);
    else
    {
        connect(reply, &QNetworkReply::finished, this, [this, reply](){
            onReplyFinished(reply);
        });
    }
}

void FaviconManager::onReplyFinished(QNetworkReply *reply)
{
    QString format = QFileInfo(getUrlAsString(reply->url())).suffix();
    QByteArray data = reply->readAll();
    if (data.isNull())
    {
        reply->deleteLater();
        return;
    }

    QImage img;
    bool success = false;

    // Handle compressed data
    if (format.compare(QStringLiteral("gzip")) == 0)
    {
        data = qUncompress(data);
        format.clear();
    }

    // Handle SVG favicons
    if (format.compare(QStringLiteral("svg")) == 0)
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
        const std::string imageFormat = format.isEmpty() ? R"()" : format.toStdString();
        success = img.load(&buffer, imageFormat.c_str());
    }

    if (success)
    {
        QIcon icon(QPixmap::fromImage(img));
        const int iconId = m_faviconStore->getFaviconIdForIconUrl(reply->url());
        FaviconData &record = m_faviconStore->getDataRecord(iconId);
        record.iconData = CommonUtil::iconToBase64(icon);

        m_faviconStore->saveDataRecord(record);

        auto it = m_iconMap.find(iconId);
        if (it != m_iconMap.end())
            it->second = icon;
        else
            m_iconMap.emplace(std::make_pair(iconId, icon));
    }
    else
        qDebug() << "FaviconManager::onReplyFinished - failed to load image from response. Format was " << format;

    reply->deleteLater();
}

QString FaviconManager::getUrlAsString(const QUrl &url) const
{
    return url.toString(QUrl::RemoveUserInfo | QUrl::RemoveQuery | QUrl::RemoveFragment);
}
