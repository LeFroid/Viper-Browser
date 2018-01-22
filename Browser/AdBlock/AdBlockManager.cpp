#include "AdBlockManager.h"
#include "AdBlockModel.h"
#include "Bitfield.h"
#include "BrowserApplication.h"
#include "DownloadItem.h"
#include "DownloadManager.h"
#include "Settings.h"
#include "WebPage.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QWebFrame>

AdBlockManager::AdBlockManager(QObject *parent) :
    QObject(parent),
    m_enabled(true),
    m_configFile(),
    m_subscriptionDir(),
    m_stylesheet(),
    m_cosmeticJSTemplate(),
    m_subscriptions(),
    m_importantBlockFilters(),
    m_blockFilters(),
    m_allowFilters(),
    m_domainStyleFilters(),
    m_domainJSFilters(),
    m_customStyleFilters(),
    m_genericHideFilters(),
    m_resourceMap(),
    m_domainStylesheetCache(24),
    m_emptyStr(),
    m_adBlockModel(nullptr)
{
    // Fetch some global settings before loading ad block data
    std::shared_ptr<Settings> settings = sBrowserApplication->getSettings();

    m_enabled = settings->getValue(QStringLiteral("AdBlockPlusEnabled")).toBool();
    m_configFile = settings->getPathValue(QStringLiteral("AdBlockPlusConfig"));
    m_subscriptionDir = settings->getPathValue(QStringLiteral("AdBlockPlusDataDir"));

    // Create data dir if it does not yet exist
    QDir subscriptionDir(m_subscriptionDir);
    if (!subscriptionDir.exists())
        subscriptionDir.mkpath(m_subscriptionDir);

    loadDynamicTemplate();
    loadUBOResources();
}

AdBlockManager::~AdBlockManager()
{
    save();
}

AdBlockManager &AdBlockManager::instance()
{
    static AdBlockManager adBlockInstance;
    return adBlockInstance;
}

void AdBlockManager::setEnabled(bool value)
{
    m_enabled = value;

    // Clear filters regardless of state, and re-extract
    // filter data from subscriptions if being set to enabled
    clearFilters();
    if (value)
        extractFilters();
}

void AdBlockManager::updateSubscriptions()
{
    if (!m_enabled)
        return;

    // Try updating the subscription if its next_update is hit
    // Check if subscription should be updated
    for (size_t i = 0; i < m_subscriptions.size(); ++i)
    {
        AdBlockSubscription *subPtr = &(m_subscriptions[i]);
        if (!subPtr)
            continue;
        const QDateTime &updateTime = subPtr->getNextUpdate();
        QDateTime now = QDateTime::currentDateTime();
        if (!updateTime.isNull() && updateTime < now)
        {
            const QUrl &srcUrl = subPtr->getSourceUrl();
            if (srcUrl.isValid() && !srcUrl.isLocalFile())
            {
                QNetworkRequest request;
                request.setUrl(srcUrl);

                DownloadItem *item = sBrowserApplication->getDownloadManager()->downloadInternal(request, m_subscriptionDir, false, true);
                connect(item, &DownloadItem::downloadFinished, [item, now, subPtr](const QString &filePath){
                    if (filePath != subPtr->getFilePath())
                    {
                        QFile oldFile(subPtr->getFilePath());
                        oldFile.remove();
                        subPtr->setFilePath(filePath);
                    }
                    item->deleteLater();
                    subPtr->setLastUpdate(now);
                    subPtr->setNextUpdate(now.addDays(7));
                });
            }
        }
    }
}

void AdBlockManager::installResource(const QUrl &url)
{
    if (!url.isValid())
        return;

    QNetworkRequest request;
    request.setUrl(url);

    DownloadManager *downloadMgr = sBrowserApplication->getDownloadManager();
    DownloadItem *item = downloadMgr->downloadInternal(request, m_subscriptionDir, false);
    connect(item, &DownloadItem::downloadFinished, this, &AdBlockManager::loadResourceFile);
}

void AdBlockManager::installSubscription(const QUrl &url)
{
    if (!url.isValid())
        return;

    QNetworkRequest request;
    request.setUrl(url);

    DownloadManager *downloadMgr = sBrowserApplication->getDownloadManager();
    DownloadItem *item = downloadMgr->downloadInternal(request, m_subscriptionDir, false);
    connect(item, &DownloadItem::downloadFinished, [=](const QString &filePath){
        AdBlockSubscription subscription(filePath);
        subscription.setSourceUrl(url);

        // Update ad block model
        int rowNum = static_cast<int>(m_subscriptions.size());
        const bool hasModel = m_adBlockModel != nullptr;
        if (hasModel)
            m_adBlockModel->beginInsertRows(QModelIndex(), rowNum, rowNum);

        m_subscriptions.push_back(std::move(subscription));

        if (hasModel)
            m_adBlockModel->endInsertRows();

        // Reload filters
        clearFilters();
        extractFilters();
    });
}

void AdBlockManager::createUserSubscription()
{
    // Associate new subscription with file "custom.txt"
    QString userFile = m_subscriptionDir;
    userFile.append(QDir::separator());
    userFile.append(QStringLiteral("custom.txt"));
    QString userFileUrl = QString("file://%1").arg(QFileInfo(userFile).absoluteFilePath());

    AdBlockSubscription subscription(userFile);
    subscription.setSourceUrl(QUrl(userFileUrl));

    // Update AdBlockModel if it is instantiated
    // Update ad block model
    int rowNum = static_cast<int>(m_subscriptions.size());
    const bool hasModel = m_adBlockModel != nullptr;
    if (hasModel)
        m_adBlockModel->beginInsertRows(QModelIndex(), rowNum, rowNum);

    m_subscriptions.push_back(std::move(subscription));

    if (hasModel)
        m_adBlockModel->endInsertRows();

    // Don't bother reloading filters until some data is set within the filter, through the editor widget
}

AdBlockModel *AdBlockManager::getModel()
{
    if (m_adBlockModel == nullptr)
        m_adBlockModel = new AdBlockModel(this);

    return m_adBlockModel;
}

const QString &AdBlockManager::getStylesheet(const QUrl &url) const
{
    // Check generic hide filters
    QString requestUrl = url.toString(QUrl::FullyEncoded).toLower();
    QString secondLevelDomain = getSecondLevelDomain(url);
    if (secondLevelDomain.isEmpty())
        secondLevelDomain = url.host();
    for (AdBlockFilter *filter : m_genericHideFilters)
    {
        if (filter->isMatch(requestUrl, requestUrl, secondLevelDomain, ElementType::Other))
            return m_emptyStr;
    }

    return m_stylesheet;
}

const QString &AdBlockManager::getDomainStylesheet(const QUrl &url)
{
    if (!m_enabled)
        return m_emptyStr;

    QString domain = getSecondLevelDomain(url);
    if (domain.isEmpty())
        domain = url.host().toLower();

    // Check for a cache hit
    std::string domainStdStr = domain.toStdString();
    if (m_domainStylesheetCache.has(domainStdStr))
        return m_domainStylesheetCache.get(domainStdStr);

    QString stylesheet;
    int numStylesheetRules = 0;
    for (AdBlockFilter *filter : m_domainStyleFilters)
    {
        if (filter->isDomainStyleMatch(domain) && !filter->isException())
        {
            stylesheet.append(filter->getEvalString() + QChar(','));
            if (numStylesheetRules > 1000)
            {
                stylesheet = stylesheet.left(stylesheet.size() - 1);
                stylesheet.append(QStringLiteral("{ display: none !important; } "));
                numStylesheetRules = 0;
            }
            ++numStylesheetRules;
        }
    }

    if (numStylesheetRules > 0)
    {
        stylesheet = stylesheet.left(stylesheet.size() - 1);
        stylesheet.append(QStringLiteral("{ display: none !important; } "));
    }

    // Check for custom stylesheet rules
    for (AdBlockFilter *filter : m_customStyleFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            stylesheet.append(filter->getEvalString());
    }

    // Insert the stylesheet into cache
    m_domainStylesheetCache.put(domainStdStr, stylesheet);
    return m_domainStylesheetCache.get(domainStdStr);
}

QString AdBlockManager::getDomainJavaScript(const QUrl &url) const
{
    if (!m_enabled)
        return QString();

    QString domain = getSecondLevelDomain(url);
    if (domain.isEmpty())
        domain = url.host().toLower();

    QString javascript;
    for (AdBlockFilter *filter : m_domainJSFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            javascript.append(filter->getEvalString());
    }

    if (!javascript.isEmpty())
    {
        QString result = m_cosmeticJSTemplate;
        result.replace(QStringLiteral("{{ADBLOCK_INTERNAL}}"), javascript);
        return result;
    }

    return QString();
}

BlockedNetworkReply *AdBlockManager::getBlockedReply(const QNetworkRequest &request)
{
    if (!m_enabled)
        return nullptr;

    QUrl requestUrlObj = request.url();

    // Attempt to get base url, defaulting to the request url if cannot be obtained
    QString baseUrl;
    QWebFrame *frame = qobject_cast<QWebFrame*>(request.originatingObject());
    if (frame != nullptr)
    {
        while (frame->parentFrame() != nullptr)
            frame = frame->parentFrame();
        baseUrl = frame->baseUrl().toString(QUrl::FullyEncoded).toLower();
    }
    else
        baseUrl = requestUrlObj.toString(QUrl::FullyEncoded).toLower();

    // Get request url in string form, as well as its domain string
    QString requestUrl = requestUrlObj.toString(QUrl::FullyEncoded).toLower();

    if (baseUrl.isEmpty())
        baseUrl = requestUrl;

    // Determine element type(s) of request
    ElementType elemType = getElementTypeMask(request, requestUrlObj.path());

    // Document type and third party type checking are done outside of getElementTypeMask
    if (requestUrl.compare(baseUrl) == 0)
        elemType |= ElementType::Document;
    QString secondLevelDomain = getSecondLevelDomain(requestUrlObj);
    if (secondLevelDomain.isEmpty())
        secondLevelDomain = requestUrlObj.host();
    if (secondLevelDomain == getSecondLevelDomain(QUrl(baseUrl)))
        elemType |= ElementType::ThirdParty;

    // Compare to filters
    for (AdBlockFilter *filter : m_importantBlockFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, secondLevelDomain, elemType))
            return new BlockedNetworkReply(request, filter->getRule(), this);
    }
    for (AdBlockFilter *filter : m_allowFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, secondLevelDomain, elemType))
            return nullptr;
    }
    for (AdBlockFilter *filter : m_blockFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, secondLevelDomain, elemType))
            return new BlockedNetworkReply(request, filter->getRule(), this);
    }

    return nullptr;
}

QString AdBlockManager::getResource(const QString &key) const
{
    return m_resourceMap.value(key);
}

int AdBlockManager::getNumSubscriptions() const
{
    return static_cast<int>(m_subscriptions.size());
}

const AdBlockSubscription *AdBlockManager::getSubscription(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return nullptr;

    return &m_subscriptions.at(index);
}

void AdBlockManager::toggleSubscriptionEnabled(int index)
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return;

    AdBlockSubscription &sub = m_subscriptions.at(index);
    sub.setEnabled(!sub.isEnabled());

    // Reset filter data
    clearFilters();
    extractFilters();
}

void AdBlockManager::reloadSubscriptions()
{
    clearFilters();
    extractFilters();
}

ElementType AdBlockManager::getElementTypeMask(const QNetworkRequest &request, const QString &requestPath)
{
    ElementType type = ElementType::None;

    if (QWebFrame *frame = qobject_cast<QWebFrame*>(request.originatingObject()))
    {
        if (frame->parentFrame() != nullptr)
            type |= ElementType::Subdocument;
    }
    if (request.rawHeader("X-Requested-With") == QByteArray("XMLHttpRequest"))
        type |= ElementType::XMLHTTPRequest;
    if (request.hasRawHeader("Sec-WebSocket-Protocol"))
        type |= ElementType::WebSocket;

    QByteArray acceptHeader = request.rawHeader(QByteArray("Accept"));
    if (acceptHeader.contains("text/css") || requestPath.endsWith(QStringLiteral(".css")))
        type |= ElementType::Stylesheet;
    if (acceptHeader.contains("text/javascript") || acceptHeader.contains("application/javascript")
            || acceptHeader.contains("script/") || requestPath.endsWith(QStringLiteral(".js")))
        type |= ElementType::Script;
    if (acceptHeader.contains("image/") || requestPath.endsWith(QStringLiteral(".jpg")) || requestPath.endsWith(QStringLiteral(".jpeg"))
            || requestPath.endsWith(QStringLiteral(".png")) || requestPath.endsWith(QStringLiteral(".gif")))
        type |= ElementType::Image;
    if (((type & ElementType::Subdocument) != ElementType::Subdocument)
            && (acceptHeader.contains("text/html") || acceptHeader.contains("application/xhtml+xml") || acceptHeader.contains("application/xml")
                || requestPath.endsWith(QStringLiteral(".html")) || requestPath.endsWith(QStringLiteral(".htm"))))
        type |= ElementType::Subdocument;
    if (acceptHeader.contains("object"))
        type |= ElementType::Object;

    return type;
}

QString AdBlockManager::getSecondLevelDomain(const QUrl &url) const
{
    const QString topLevelDomain = url.topLevelDomain();
    const QString host = url.host();

    if (topLevelDomain.isEmpty() || host.isEmpty())
        return QString();

    QString domain = host.left(host.size() - topLevelDomain.size());

    if (domain.count(QChar('.')) == 0)
        return host;

    while (domain.count(QChar('.')) != 0)
        domain = domain.mid(domain.indexOf(QChar('.')) + 1);

    return domain + topLevelDomain;
}

void AdBlockManager::loadDynamicTemplate()
{
    QFile templateFile(QStringLiteral(":/AdBlock.js"));
    if (!templateFile.open(QIODevice::ReadOnly))
        return;

    m_cosmeticJSTemplate = templateFile.readAll();
    templateFile.close();
}

void AdBlockManager::loadUBOResources()
{
    QDir resourceDir(QString("%1%2%3").arg(m_subscriptionDir).arg(QDir::separator()).arg(QStringLiteral("resources")));
    if (!resourceDir.exists())
        resourceDir.mkpath(QStringLiteral("."));

    // Iterate through files in directory, loading into m_resourceMap
    QDirIterator resourceItr(resourceDir.absolutePath(), QDir::Files);
    while (resourceItr.hasNext())
    {
        loadResourceFile(resourceItr.next());
    }
}

void AdBlockManager::loadResourceFile(const QString &path)
{
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;

    //TODO: handle non-javascript types properly for redirect filter option
    bool readingValue = false;
    QString currentKey, mimeType;
    QByteArray currentValue;
    QList<QByteArray> contents = f.readAll().split('\n');
    f.close();
    for (int i = 0; i < contents.size(); ++i)
    {
        const QByteArray &line = contents.at(i);
        if ((!readingValue && line.isEmpty()) || line.startsWith('#'))
            continue;

        // Extract key from line if not loading a value associated with a key
        if (!readingValue)
        {
            int sepIdx = line.indexOf(' ');
            if (sepIdx < 0)
                currentKey = line;
            else
            {
                currentKey = line.left(sepIdx);
                mimeType = line.mid(sepIdx + 1);
            }
            readingValue = true;
        }
        else
        {
            // Append currentValue with line if not empty.
            if (!line.isEmpty())
            {
                currentValue.append(line);
                if (mimeType.contains(QStringLiteral("javascript")))
                    currentValue.append('\n');
            }
            else
            {
                // Insert key-value pair into map once an empty line is reached and search for next key
                m_resourceMap.insert(currentKey, QString(currentValue));
                currentValue.clear();
                readingValue = false;
            }
        }
    }
}

void AdBlockManager::loadSubscriptions()
{
    if (!m_enabled)
        return;

    QFile configFile(m_configFile);
    if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
        return;

    // Attempt to parse config/subscription info file
    QByteArray configData = configFile.readAll();
    configFile.close();

    // Add subscriptions to container
    QJsonDocument configDoc(QJsonDocument::fromJson(configData));
    QJsonObject configObj = configDoc.object();
    QJsonObject subscriptionObj;
    for (auto it = configObj.begin(); it != configObj.end(); ++it)
    {
        AdBlockSubscription subscription(it.key());

        subscriptionObj = it.value().toObject();
        subscription.setEnabled(subscriptionObj.value(QStringLiteral("enabled")).toBool());

        // Get last update as unix epoch value
        bool ok;
        uint lastUpdateUInt = subscriptionObj.value(QStringLiteral("last_update")).toVariant().toUInt(&ok);
        QDateTime lastUpdate = (ok && lastUpdateUInt > 0 ? QDateTime::fromTime_t(lastUpdateUInt) : QDateTime::currentDateTime());
        subscription.setLastUpdate(lastUpdate);

        // Attempt to get next update time as unix epoch value
        uint nextUpdateUInt = subscriptionObj.value(QStringLiteral("next_update")).toVariant().toUInt(&ok);
        if (ok && nextUpdateUInt > 0)
        {
            QDateTime nextUpdate = QDateTime::fromTime_t(nextUpdateUInt);
            subscription.setNextUpdate(nextUpdate);
        }

        // Get source URL of the subscription
        QString source = subscriptionObj.value(QStringLiteral("source")).toString();
        if (!source.isEmpty())
            subscription.setSourceUrl(QUrl(source));

        m_subscriptions.push_back(std::move(subscription));
    }

    extractFilters();
}

void AdBlockManager::clearFilters()
{
    m_importantBlockFilters.clear();
    m_allowFilters.clear();
    m_blockFilters.clear();
    m_stylesheet.clear();
    m_domainStyleFilters.clear();
    m_domainJSFilters.clear();
    m_customStyleFilters.clear();
    m_genericHideFilters.clear();
}

void AdBlockManager::extractFilters()
{
    // Used to store css rules for the global stylesheet and domain-specific stylesheets
    QHash<QString, AdBlockFilter*> stylesheetFilterMap;
    QHash<QString, AdBlockFilter*> stylesheetExceptionMap;

    // Used to remove bad filters (badfilter option from uBlock)
    QSet<QString> badFilters, badHideFilters;

    // Setup global stylesheet string
    m_stylesheet = QStringLiteral("<style>");

    for (AdBlockSubscription &s : m_subscriptions)
    {
        // calling load() does nothing if subscription is disabled
        s.load();

        // Add filters to appropriate containers
        int numFilters = s.getNumFilters();
        for (int i = 0; i < numFilters; ++i)
        {
            AdBlockFilter *filter = s.getFilter(i);
            if (!filter)
                continue;

            if (filter->getCategory() == FilterCategory::Stylesheet)
            {
                if (filter->isException())
                    stylesheetExceptionMap.insert(filter->getEvalString(), filter);
                else
                    stylesheetFilterMap.insert(filter->getEvalString(), filter);
            }
            else if (filter->getCategory() == FilterCategory::StylesheetJS)
            {
                m_domainJSFilters.push_back(filter);
            }
            else if (filter->getCategory() == FilterCategory::StylesheetCustom)
            {
                m_customStyleFilters.push_back(filter);
            }
            else if (filter->hasElementType(filter->m_blockedTypes, ElementType::BadFilter))
            {
                badFilters.insert(filter->getRule());
            }
            else
            {
                if (filter->isException())
                {
                    if (filter->hasElementType(filter->m_blockedTypes, ElementType::GenericHide))
                        m_genericHideFilters.push_back(filter);
                    else
                        m_allowFilters.push_back(filter);
                }
                else if (filter->isImportant())
                {
                    if (filter->hasElementType(filter->m_blockedTypes, ElementType::GenericHide))
                        badHideFilters.insert(filter->getRule());
                    else
                        m_importantBlockFilters.push_back(filter);
                }
                else
                    m_blockFilters.push_back(filter);
            }
        }
    }

    // Remove bad filters from m_allowFilters, m_blockFilters, m_genericHideFilters
    for (auto it = m_allowFilters.begin(); it != m_allowFilters.end(); ++it)
    {
        if (badFilters.contains((*it)->getRule()))
            m_allowFilters.erase(it);
    }
    for (auto it = m_blockFilters.begin(); it != m_blockFilters.end(); ++it)
    {
        if (badFilters.contains((*it)->getRule()))
            m_blockFilters.erase(it);
    }
    for (auto it = m_genericHideFilters.begin(); it != m_genericHideFilters.end(); ++it)
    {
        if (badHideFilters.contains((*it)->getRule()))
            m_genericHideFilters.erase(it);
    }

    // Parse stylesheet exceptions
    QHashIterator<QString, AdBlockFilter*> it(stylesheetExceptionMap);
    while (it.hasNext())
    {
        it.next();

        // Ignore the exception if the blocking rule does not exist
        if (!stylesheetFilterMap.contains(it.key()))
            continue;

        AdBlockFilter *filter = it.value();
        stylesheetFilterMap.value(it.key())->m_domainWhitelist.unite(filter->m_domainBlacklist);
    }

    // Parse stylesheet blocking rules
    it = QHashIterator<QString, AdBlockFilter*>(stylesheetFilterMap);
    int numStylesheetRules = 0;
    while (it.hasNext())
    {
        it.next();
        AdBlockFilter *filter = it.value();

        if (filter->hasDomainRules())
        {
            m_domainStyleFilters.push_back(filter);
            continue;
        }

        if (numStylesheetRules > 1000)
        {
            m_stylesheet = m_stylesheet.left(m_stylesheet.size() - 1);
            m_stylesheet.append(QStringLiteral("{ display: none !important; } "));
            numStylesheetRules = 0;
        }

        m_stylesheet.append(filter->getEvalString() + QChar(','));
        ++numStylesheetRules;
    }

    // Complete global stylesheet string
    if (numStylesheetRules > 0)
    {
        m_stylesheet = m_stylesheet.left(m_stylesheet.size() - 1);
        m_stylesheet.append(QStringLiteral("{ display: none !important; } "));
    }
    m_stylesheet.append(QStringLiteral("</style>"));
}

void AdBlockManager::save()
{
    QFile configFile(m_configFile);
    if (!configFile.open(QIODevice::WriteOnly))
        return;

    // Construct configuration JSON object
    // Format: { "/path/to/subscription1.txt": { subscription object 1 },
    //           "/path/to/subscription2.txt": { subscription object 2 } }
    // Subscription object format: { "enabled": (true|false), "last_update": (timestamp),
    //                               "next_update": (timestamp), "source": "origin_url" }
    QJsonObject configObj;
    for (auto it = m_subscriptions.cbegin(); it != m_subscriptions.cend(); ++it)
    {
        QJsonObject subscriptionObj;
        subscriptionObj.insert(QStringLiteral("enabled"), it->isEnabled());
        subscriptionObj.insert(QStringLiteral("last_update"), QJsonValue::fromVariant(QVariant(it->getLastUpdate().toTime_t())));
        subscriptionObj.insert(QStringLiteral("next_update"), QJsonValue::fromVariant(QVariant(it->getNextUpdate().toTime_t())));
        subscriptionObj.insert(QStringLiteral("source"), it->getSourceUrl().toString(QUrl::FullyEncoded));

        configObj.insert(it->getFilePath(), QJsonValue(subscriptionObj));
    }

    QJsonDocument configDoc(configObj);
    configFile.write(configDoc.toJson());
    configFile.close();
}
