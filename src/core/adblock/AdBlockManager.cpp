#include "AdBlockManager.h"
#include "AdBlockLog.h"
#include "AdBlockModel.h"
#include "Bitfield.h"
#include "BrowserApplication.h"
#include "InternalDownloadItem.h"
#include "DownloadManager.h"
#include "Settings.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkRequest>

#include <QDebug>

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
    m_blockFiltersByPattern(),
    m_blockFiltersByDomain(),
    m_allowFilters(),
    m_domainStyleFilters(),
    m_domainJSFilters(),
    m_customStyleFilters(),
    m_genericHideFilters(),
    m_cspFilters(),
    m_resourceMap(),
    m_resourceContentTypeMap(),
    m_domainStylesheetCache(24),
    m_jsInjectionCache(24),
    m_emptyStr(),
    m_adBlockModel(nullptr),
    m_numRequestsBlocked(0),
    m_pageAdBlockCount(),
    m_log(nullptr)
{
    // Fetch some global settings before loading ad block data
    Settings *settings = sBrowserApplication->getSettings();

    m_enabled = settings->getValue(BrowserSetting::AdBlockPlusEnabled).toBool();
    m_configFile = settings->getPathValue(BrowserSetting::AdBlockPlusConfig);
    m_subscriptionDir = settings->getPathValue(BrowserSetting::AdBlockPlusDataDir);

    // Create data dir if it does not yet exist
    QDir subscriptionDir(m_subscriptionDir);
    if (!subscriptionDir.exists())
        subscriptionDir.mkpath(m_subscriptionDir);

    loadDynamicTemplate();
    loadUBOResources();

    // Instantiate the logger
    m_log = new AdBlockLog(this);
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

                InternalDownloadItem *item = sBrowserApplication->getDownloadManager()->downloadInternal(request, m_subscriptionDir, false, true);
                connect(item, &InternalDownloadItem::downloadFinished, [item, now, subPtr](const QString &filePath){
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
    InternalDownloadItem *item = downloadMgr->downloadInternal(request, m_subscriptionDir + QDir::separator() + QString("resources"), false);
    connect(item, &InternalDownloadItem::downloadFinished, this, &AdBlockManager::loadResourceFile);
}

void AdBlockManager::installSubscription(const QUrl &url)
{
    if (!url.isValid())
        return;

    QNetworkRequest request;
    request.setUrl(url);

    DownloadManager *downloadMgr = sBrowserApplication->getDownloadManager();
    InternalDownloadItem *item = downloadMgr->downloadInternal(request, m_subscriptionDir, false);
    connect(item, &InternalDownloadItem::downloadFinished, [=](const QString &filePath){
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
    userFile.append(QLatin1String("custom.txt"));
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

void AdBlockManager::loadStarted(const QUrl &url)
{
    m_pageAdBlockCount[url] = 0;
}

AdBlockLog *AdBlockManager::getLog() const
{
    return m_log;
}

AdBlockModel *AdBlockManager::getModel()
{
    if (m_adBlockModel == nullptr)
        m_adBlockModel = new AdBlockModel(this);

    return m_adBlockModel;
}

const QString &AdBlockManager::getStylesheet(const URL &url) const
{
    // Check generic hide filters
    QString requestUrl = url.toString(URL::FullyEncoded).toLower();
    QString secondLevelDomain = url.getSecondLevelDomain();
    if (secondLevelDomain.isEmpty())
        secondLevelDomain = url.host();
    for (AdBlockFilter *filter : m_genericHideFilters)
    {
        if (filter->isMatch(requestUrl, requestUrl, secondLevelDomain, ElementType::Other))
            return m_emptyStr;
    }

    return m_stylesheet;
}

const QString &AdBlockManager::getDomainStylesheet(const URL &url)
{
    if (!m_enabled)
        return m_emptyStr;

    QString domain = url.host().toLower();
    //QString sld = url.getSecondLevelDomain();
    if (domain.startsWith(QLatin1String("www.")))
        domain = domain.mid(4);

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
                stylesheet.append(QLatin1String("{ display: none !important; } "));
                numStylesheetRules = 0;
            }
            ++numStylesheetRules;
        }
    }

    if (numStylesheetRules > 0)
    {
        stylesheet = stylesheet.left(stylesheet.size() - 1);
        stylesheet.append(QLatin1String("{ display: none !important; } "));
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

const QString &AdBlockManager::getDomainJavaScript(const URL &url)
{
    if (!m_enabled)
        return m_emptyStr;

    const static QString cspScript = QStringLiteral("(function() {\n"
                                       "var doc = document;\n"
                                       "if (!doc.head) { \n"
                                       " document.onreadystatechange = function(){ \n"
                                       "  if (document.readyState == 'interactive') { \n"
                                       "   var meta = document.createElement('meta');\n"
                                       "   meta.setAttribute('http-equiv', 'Content-Security-Policy');\n"
                                       "   meta.setAttribute('content', \"%1\");\n"
                                       "   document.head.appendChild(meta);\n"
                                       "  }\n"
                                       " }\n"
                                       " return;\n"
                                       "}\n"
                                       "var meta = doc.createElement('meta');\n"
                                       "meta.setAttribute('http-equiv', 'Content-Security-Policy');\n"
                                       "meta.setAttribute('content', \"%1\");\n"
                                       "doc.head.appendChild(meta);\n"
                                   "})();");
    bool usedCspScript = false;

    QString domain = url.host().toLower();
    if (domain.startsWith(QLatin1String("www.")))
        domain = domain.mid(4);
    if (domain.isEmpty())
        domain = url.getSecondLevelDomain();

    QString requestUrl = url.toString(QUrl::FullyEncoded).toLower();

    // Check for cache hit
    std::string requestHostStdStr = url.host().toLower().toStdString();
    if (requestHostStdStr.empty())
        requestHostStdStr = domain.toStdString();

    if (m_jsInjectionCache.has(requestHostStdStr))
        return m_jsInjectionCache.get(requestHostStdStr);

    QString javascript;

    std::vector<QString> cspDirectives;
    for (AdBlockFilter *filter : m_domainJSFilters)
    {
        if (filter->isDomainStyleMatch(domain))
            javascript.append(filter->getEvalString());
    }

    auto filterCSPCheck = [&](std::deque<AdBlockFilter*> &filterContainer) {
        for (AdBlockFilter *filter : filterContainer)
        {
            if (usedCspScript)
                return;

            if (filter->hasElementType(filter->m_blockedTypes, ElementType::InlineScript) && filter->isMatch(requestUrl, requestUrl, domain, ElementType::InlineScript))
            {
                cspDirectives.push_back(QLatin1String("script-src 'unsafe-eval' * blob: data:"));
                usedCspScript = true;
                return;
            }
        }
    };

    filterCSPCheck(m_importantBlockFilters);

    auto it = m_blockFiltersByDomain.find(domain);
    if (it != m_blockFiltersByDomain.end())
    {
        filterCSPCheck(*it);
    }

    filterCSPCheck(m_blockFilters);
    filterCSPCheck(m_blockFiltersByPattern);

    for (AdBlockFilter *filter : m_cspFilters)
    {
        if (!filter->isException() && filter->isMatch(requestUrl, requestUrl, domain, ElementType::CSP))
        {
            cspDirectives.push_back(filter->getContentSecurityPolicy());
        }
    }

    QString result;
    if (!cspDirectives.empty())
    {
        QString cspConcatenated;
        for (size_t i = 0; i < cspDirectives.size(); ++i)
        {
            cspConcatenated.append(cspDirectives.at(i));
            if (i + 1 < cspDirectives.size())
                cspConcatenated.append(QLatin1String("; "));
        }
        cspConcatenated.replace(QLatin1String("\""), QLatin1String("\\\""));
        javascript.append(cspScript.arg(cspConcatenated));
    }

    if (!javascript.isEmpty())
    {
        result = m_cosmeticJSTemplate;
        result.replace(QLatin1String("{{ADBLOCK_INTERNAL}}"), javascript);
    }

    m_jsInjectionCache.put(requestHostStdStr, result);
    return m_jsInjectionCache.get(requestHostStdStr);
}

bool AdBlockManager::shouldBlockRequest(QWebEngineUrlRequestInfo &info)
{
    if (!m_enabled)
        return false;

    // Check if scheme is whitelisted
    if (isSchemeWhitelisted(info.requestUrl().scheme().toLower()))
        return false;

    // Get request URL and the originating URL
    QUrl firstPartyUrl = info.firstPartyUrl();
    QString requestUrl = info.requestUrl().toString(QUrl::FullyEncoded).toLower();
    QString baseUrl = getSecondLevelDomain(firstPartyUrl).toLower();

    // Get request domain
    QString domain = info.requestUrl().host().toLower();
    if (domain.startsWith(QLatin1String("www.")))
        domain = domain.mid(4);
    if (domain.isEmpty())
        domain = getSecondLevelDomain(info.requestUrl());

    if (baseUrl.isEmpty())
        baseUrl = firstPartyUrl.host().toLower();

    // Convert QWebEngine request type to AdBlockFilter request type
    ElementType elemType = getRequestType(info);

    // Compare to filters
    for (auto it = m_importantBlockFilters.begin(); it != m_importantBlockFilters.end(); ++it)
    {
        AdBlockFilter *filter = *it;
        if (filter->isMatch(baseUrl, requestUrl, domain, elemType))
        {
            ++m_numRequestsBlocked;
            ++m_pageAdBlockCount[firstPartyUrl];

            it = m_importantBlockFilters.erase(it);
            m_importantBlockFilters.push_front(filter);

            if (filter->isRedirect())
            {
                info.redirect(QUrl(QString("blocked:%1").arg(filter->getRedirectName())));
                m_log->addEntry(AdBlockFilterAction::Redirect, info.firstPartyUrl(), info.requestUrl(), elemType, filter->getRule(), QDateTime::currentDateTime());
                return false;
            }

            m_log->addEntry(AdBlockFilterAction::Block, info.firstPartyUrl(), info.requestUrl(), elemType, filter->getRule(), QDateTime::currentDateTime());
            return true;
        }
    }

    // Look for a matching blocking filter before iterating through the allowed filter list, to avoid wasted resources
    AdBlockFilter *matchingBlockFilter = nullptr;

    auto checkFiltersForMatch = [&](std::deque<AdBlockFilter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end(); ++it)
        {
            AdBlockFilter *filter = *it;
            if (filter->isMatch(baseUrl, requestUrl, domain, elemType))
            {
                matchingBlockFilter = filter;
                it = filterContainer.erase(it);
                filterContainer.push_front(filter);
                return;
            }
        }
    };

    auto itr = m_blockFiltersByDomain.find(getSecondLevelDomain(info.requestUrl()));
    if (itr != m_blockFiltersByDomain.end())
        checkFiltersForMatch(*itr);

    if (matchingBlockFilter == nullptr)
        checkFiltersForMatch(m_blockFilters);

    if (matchingBlockFilter == nullptr)
        checkFiltersForMatch(m_blockFiltersByPattern);

    if (matchingBlockFilter == nullptr)
        return false;

    for (AdBlockFilter *filter : m_allowFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, domain, elemType))
        {
            m_log->addEntry(AdBlockFilterAction::Allow, info.firstPartyUrl(), info.requestUrl(), elemType, filter->getRule(), QDateTime::currentDateTime());
            return false;
        }
    }

    // If we reach this point, then the matching block filter is applied to the request
    ++m_numRequestsBlocked;
    ++m_pageAdBlockCount[firstPartyUrl];

    if (matchingBlockFilter->isRedirect())
    {
        info.redirect(QUrl(QString("blocked:%1").arg(matchingBlockFilter->getRedirectName())));
        m_log->addEntry(AdBlockFilterAction::Redirect, info.firstPartyUrl(), info.requestUrl(), elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
        return false;
    }

    m_log->addEntry(AdBlockFilterAction::Block, info.firstPartyUrl(), info.requestUrl(), elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
    return true;
}

quint64 AdBlockManager::getRequestsBlockedCount() const
{
    return m_numRequestsBlocked;
}

int AdBlockManager::getNumberAdsBlocked(const QUrl &url)
{
    auto it = m_pageAdBlockCount.find(url);
    if (it != m_pageAdBlockCount.end())
        return *it;

    return 0;
}

QString AdBlockManager::getResource(const QString &key) const
{
    return m_resourceMap.value(key);
}

QString AdBlockManager::getResourceContentType(const QString &key) const
{
    return m_resourceContentTypeMap.value(key);
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
    reloadSubscriptions();
}

void AdBlockManager::removeSubscription(int index)
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return;

    // Point iterator to subscription
    auto it = m_subscriptions.cbegin() + index;

    // Delete the subscription file before removal
    QFile subFile(it->getFilePath());
    if (subFile.exists())
    {
        if (!subFile.remove())
            qDebug() << "[Advertisement Blocker]: Could not remove subscription file " << subFile.fileName();
    }

    m_subscriptions.erase(it);

    reloadSubscriptions();
}

void AdBlockManager::reloadSubscriptions()
{
    m_domainStylesheetCache.clear();
    m_jsInjectionCache.clear();

    clearFilters();
    extractFilters();
}

bool AdBlockManager::isSchemeWhitelisted(const QString &scheme) const
{
    const std::array<QString, 4> whitelistedSchemes {
            QLatin1String("blocked"),
            QLatin1String("file"),
            QLatin1String("qrc"),
            QLatin1String("viper")
    };

    for (const QString &allowedScheme : whitelistedSchemes)
    {
        if (allowedScheme.compare(scheme) == 0)
            return true;
    }

    return false;
}

ElementType AdBlockManager::getRequestType(const QWebEngineUrlRequestInfo &info) const
{
    const QUrl firstPartyUrl = info.firstPartyUrl();
    const QUrl requestUrl = info.requestUrl();
    const QString requestUrlStr = info.requestUrl().toString(QUrl::FullyEncoded).toLower();

    ElementType elemType = ElementType::None;
    switch (info.resourceType())
    {
        case QWebEngineUrlRequestInfo::ResourceTypeMainFrame:
            elemType |= ElementType::Document;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeSubFrame:
            elemType |= ElementType::Subdocument;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeStylesheet:
            elemType |= ElementType::Stylesheet;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeScript:
            elemType |= ElementType::Script;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeImage:
            elemType |= ElementType::Image;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeSubResource:
            if (requestUrlStr.endsWith(QLatin1String("htm"))
                || requestUrlStr.endsWith(QLatin1String("html"))
                || requestUrlStr.endsWith(QLatin1String("xml")))
            {
                elemType |= ElementType::Subdocument;
            }
            else
                elemType |= ElementType::Other;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeXhr:
            elemType |= ElementType::XMLHTTPRequest;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypePing:
            elemType |= ElementType::Ping;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypePluginResource:
            elemType |= ElementType::ObjectSubrequest;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeObject:
            elemType |= ElementType::Object;
            break;
        case QWebEngineUrlRequestInfo::ResourceTypeFontResource:
        case QWebEngineUrlRequestInfo::ResourceTypeMedia:
        case QWebEngineUrlRequestInfo::ResourceTypeWorker:
        case QWebEngineUrlRequestInfo::ResourceTypeSharedWorker:
        case QWebEngineUrlRequestInfo::ResourceTypeServiceWorker:
        case QWebEngineUrlRequestInfo::ResourceTypePrefetch:
        case QWebEngineUrlRequestInfo::ResourceTypeFavicon:
        case QWebEngineUrlRequestInfo::ResourceTypeCspReport:
        default:
            elemType |= ElementType::Other;
            break;
    }

    // Check for third party request type
    if (firstPartyUrl.isEmpty() || (getSecondLevelDomain(requestUrl) != getSecondLevelDomain(firstPartyUrl)))
        elemType |= ElementType::ThirdParty;

    return elemType;
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
    QFile templateFile(QLatin1String(":/AdBlock.js"));
    if (!templateFile.open(QIODevice::ReadOnly))
        return;

    m_cosmeticJSTemplate = templateFile.readAll();
    templateFile.close();
}

void AdBlockManager::loadUBOResources()
{
    QDir resourceDir(QString("%1%2%3").arg(m_subscriptionDir).arg(QDir::separator()).arg(QLatin1String("resources")));
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
                m_resourceContentTypeMap.insert(currentKey, mimeType);
            }
            readingValue = true;
        }
        else
        {
            // Append currentValue with line if not empty.
            if (!line.isEmpty())
            {
                currentValue.append(line);
                if (mimeType.contains(QLatin1String("javascript")))
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
        const QString key = it.key();
        if (key.compare(QLatin1String("requests_blocked")) == 0)
        {
            m_numRequestsBlocked = it.value().toString().toULongLong();
            continue;
        }
        AdBlockSubscription subscription(key);

        subscriptionObj = it.value().toObject();
        subscription.setEnabled(subscriptionObj.value(QLatin1String("enabled")).toBool());

        // Get last update as unix epoch value
        bool ok;
        quint64 lastUpdateUInt = subscriptionObj.value(QLatin1String("last_update")).toVariant().toULongLong(&ok);
        QDateTime lastUpdate = (ok && lastUpdateUInt > 0 ? QDateTime::fromSecsSinceEpoch(lastUpdateUInt) : QDateTime::currentDateTime());
        subscription.setLastUpdate(lastUpdate);

        // Attempt to get next update time as unix epoch value
        quint64 nextUpdateUInt = subscriptionObj.value(QLatin1String("next_update")).toVariant().toULongLong(&ok);
        if (ok && nextUpdateUInt > 0)
        {
            QDateTime nextUpdate = QDateTime::fromSecsSinceEpoch(nextUpdateUInt);
            subscription.setNextUpdate(nextUpdate);
        }

        // Get source URL of the subscription
        QString source = subscriptionObj.value(QLatin1String("source")).toString();
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
    m_blockFiltersByPattern.clear();
    m_blockFiltersByDomain.clear();
    m_stylesheet.clear();
    m_domainStyleFilters.clear();
    m_domainJSFilters.clear();
    m_customStyleFilters.clear();
    m_genericHideFilters.clear();
    m_cspFilters.clear();
}

void AdBlockManager::extractFilters()
{
    // Used to store css rules for the global stylesheet and domain-specific stylesheets
    QHash<QString, AdBlockFilter*> stylesheetFilterMap;
    QHash<QString, AdBlockFilter*> stylesheetExceptionMap;

    // Used to remove bad filters (badfilter option from uBlock)
    QSet<QString> badFilters, badHideFilters;

    // Setup global stylesheet string
    m_stylesheet = QLatin1String("<style>");

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
            else if (filter->hasElementType(filter->m_blockedTypes, ElementType::CSP))
            {
                if (!filter->hasElementType(filter->m_blockedTypes, ElementType::PopUp)) // Temporary workaround for issues with popup types
                    m_cspFilters.push_back(filter);
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
                else if (filter->getCategory() == FilterCategory::StringContains)
                {
                    m_blockFiltersByPattern.push_back(filter);
                }
                else if (filter->getCategory() == FilterCategory::Domain)
                {
                    //const QString &filterDomain = filter->getEvalString();
                    const QString filterDomain = getSecondLevelDomain(QUrl::fromUserInput(filter->getEvalString()));
                    auto it = m_blockFiltersByDomain.find(filterDomain);
                    if (it != m_blockFiltersByDomain.end())
                    {
                        it->push_back(filter);
                    }
                    else
                    {
                        std::deque<AdBlockFilter*> queue;
                        queue.push_back(filter);
                        m_blockFiltersByDomain.insert(filterDomain, queue);
                    }
                }
                else
                {
                    m_blockFilters.push_back(filter);
                }
            }
        }
    }

    // Remove bad filters from all applicable filter containers
    auto removeBadFiltersFromVector = [&badFilters](std::vector<AdBlockFilter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end();)
        {
            if (badFilters.contains((*it)->getRule()))
                it = filterContainer.erase(it);
            else
                ++it;
        }
    };
    auto removeBadFiltersFromDeque = [&badFilters](std::deque<AdBlockFilter*> &filterContainer) {
        for (auto it = filterContainer.begin(); it != filterContainer.end();)
        {
            if (badFilters.contains((*it)->getRule()))
                it = filterContainer.erase(it);
            else
                ++it;
        }
    };

    removeBadFiltersFromVector(m_allowFilters);

    removeBadFiltersFromDeque(m_blockFilters);
    removeBadFiltersFromDeque(m_blockFiltersByPattern);

    for (std::deque<AdBlockFilter*> &queue : m_blockFiltersByDomain)
    {
        removeBadFiltersFromDeque(queue);
    }

    removeBadFiltersFromVector(m_cspFilters);
    removeBadFiltersFromVector(m_genericHideFilters);

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
            m_stylesheet.append(QLatin1String("{ display: none !important; } "));
            numStylesheetRules = 0;
        }

        m_stylesheet.append(filter->getEvalString() + QChar(','));
        ++numStylesheetRules;
    }

    // Complete global stylesheet string
    if (numStylesheetRules > 0)
    {
        m_stylesheet = m_stylesheet.left(m_stylesheet.size() - 1);
        m_stylesheet.append(QLatin1String("{ display: none !important; } "));
    }
    m_stylesheet.append(QLatin1String("</style>"));
}

void AdBlockManager::save()
{
    QFile configFile(m_configFile);
    if (!configFile.open(QIODevice::WriteOnly))
        return;

    // Construct configuration JSON object
    // Format:
    // {
    //     "requests_blocked": "total requests blocked",
    //     "/path/to/subscription1.txt": { subscription object 1 },
    //     "/path/to/subscription2.txt": { subscription object 2 }
    // }
    // Subscription object format: { "enabled": (true|false), "last_update": (timestamp),
    //                               "next_update": (timestamp), "source": "origin_url" }
    QJsonObject configObj;
    configObj.insert(QLatin1String("requests_blocked"), QJsonValue(QString::number(m_numRequestsBlocked)));
    for (auto it = m_subscriptions.cbegin(); it != m_subscriptions.cend(); ++it)
    {
        QJsonObject subscriptionObj;
        subscriptionObj.insert(QLatin1String("enabled"), it->isEnabled());
        subscriptionObj.insert(QLatin1String("last_update"), QJsonValue::fromVariant(QVariant(it->getLastUpdate().toSecsSinceEpoch())));
        subscriptionObj.insert(QLatin1String("next_update"), QJsonValue::fromVariant(QVariant(it->getNextUpdate().toSecsSinceEpoch())));
        subscriptionObj.insert(QLatin1String("source"), it->getSourceUrl().toString(QUrl::FullyEncoded));

        configObj.insert(it->getFilePath(), QJsonValue(subscriptionObj));
    }

    QJsonDocument configDoc(configObj);
    configFile.write(configDoc.toJson());
    configFile.close();
}
