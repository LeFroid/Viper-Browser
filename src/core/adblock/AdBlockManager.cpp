#include "AdBlockManager.h"
#include "AdBlockLog.h"
#include "AdBlockModel.h"
#include "AdBlockRequestHandler.h"
#include "Bitfield.h"
#include "InternalDownloadItem.h"
#include "DownloadManager.h"
#include "SchemeRegistry.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkRequest>
#include <QtGlobal>

#include <QDebug>

namespace adblock
{

AdBlockManager::AdBlockManager(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QObject(parent),
    m_filterContainer(),
    m_downloadManager(nullptr),
    m_enabled(true),
    m_configFile(),
    m_subscriptionDir(),
    m_cosmeticJSTemplate(),
    m_subscriptions(),
    m_resourceAliasMap (),
    m_resourceMap(),
    m_resourceContentTypeMap(),
    m_domainStylesheetCache(24),
    m_jsInjectionCache(24),
    m_emptyStr(),
    m_adBlockModel(nullptr),
    m_log(nullptr),
    m_requestHandler(nullptr)
{
    setObjectName(QLatin1String("AdBlockManager"));

    m_downloadManager = serviceLocator.getServiceAs<DownloadManager>("DownloadManager");

    if (Settings *settings = serviceLocator.getServiceAs<Settings>("Settings"))
    {
        m_enabled = settings->getValue(BrowserSetting::AdBlockPlusEnabled).toBool();
        m_configFile = settings->getPathValue(BrowserSetting::AdBlockPlusConfig);
        m_subscriptionDir = settings->getPathValue(BrowserSetting::AdBlockPlusDataDir);

        // Subscribe to settings event notifications
        connect(settings, &Settings::settingChanged, this, &AdBlockManager::onSettingChanged);
    }

    // Create data dir if it does not yet exist
    QDir subscriptionDir(m_subscriptionDir);
    if (!subscriptionDir.exists())
        subscriptionDir.mkpath(m_subscriptionDir);

    loadDynamicTemplate();
    loadUBOResources();

    // Instantiate the logger
    m_log = new AdBlockLog(this);

    // Instantiate the network request handler
    m_requestHandler = new RequestHandler(m_filterContainer, m_log, this);
}

AdBlockManager::~AdBlockManager()
{
    save();
}

void AdBlockManager::setEnabled(bool value)
{
    if (m_enabled == value)
        return;

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
        Subscription *subPtr = &(m_subscriptions[i]);
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

                InternalDownloadItem *item = m_downloadManager->downloadInternal(request, m_subscriptionDir, false, true);
                connect(item, &InternalDownloadItem::downloadFinished, this, [item, now, subPtr](const QString &filePath) {
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

    InternalDownloadItem *item = m_downloadManager->downloadInternal(request, m_subscriptionDir + QDir::separator() + QString("resources"), false);
    connect(item, &InternalDownloadItem::downloadFinished, this, &AdBlockManager::loadResourceFile);
}

void AdBlockManager::installSubscription(const QUrl &url)
{
    if (!url.isValid())
        return;

    QNetworkRequest request;
    request.setUrl(url);

    InternalDownloadItem *item = m_downloadManager->downloadInternal(request, m_subscriptionDir, false);
    connect(item, &InternalDownloadItem::downloadFinished, this, [this, url](const QString &filePath){
        Subscription subscription(filePath);
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

    Subscription subscription(userFile);
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
    m_requestHandler->loadStarted(url);
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

    if (m_filterContainer.hasGenericHideFilter(requestUrl, secondLevelDomain))
        return m_emptyStr;

    return m_filterContainer.getCombinedFilterStylesheet();
}

const QString &AdBlockManager::getDomainStylesheet(const URL &url)
{
    if (!m_enabled)
        return m_emptyStr;

    const QString domain = url.host().toLower();

    // Check for a cache hit
    std::string domainStdStr = domain.toStdString();
    if (m_domainStylesheetCache.has(domainStdStr))
        return m_domainStylesheetCache.get(domainStdStr);

    const static QString styleScript = QStringLiteral("(function() {\n"
                                       "var doc = document;\n"
                                       "if (!doc.head) { \n"
                                       " document.onreadystatechange = function(){ \n"
                                       "  if (document.readyState == 'interactive') { \n"
                                       "   var sheet = document.createElement('style');\n"
                                       "   sheet.type = 'text/css';\n"
                                       "   sheet.innerHTML = '%1';\n"
                                       "   document.head.appendChild(sheet);\n"
                                       "  }\n"
                                       " }\n"
                                       " return;\n"
                                       "}\n"
                                       "var sheet = document.createElement('style');\n"
                                       "sheet.type = 'text/css';\n"
                                       "sheet.innerHTML = '%1';\n"
                                       "doc.head.appendChild(sheet);\n"
                                   "})();");
    const static QString styleScriptAlt = QStringLiteral("(function() {\n"
                                          "  const queries = [ %1 ];\n"
                                          "  document.onreadystatechange = function() {\n"
                                          "    if (document.readyState == 'interactive' || document.readyState == 'complete') { \n"
                                          "      queries.forEach((q) => { \n"
                                          "        try { const hits = document.querySelectorAll(q); \n"
                                          "        hits.forEach((el) => { el.style.display = 'none'; }); } catch (ex) { console.error(`Cosmetic filter script: could not query for ${q} - ${ex.message}`); }\n"
                                          "      });\n"
                                          "    }\n"
                                          "  }\n"
                                          "})();");

    QString stylesheet;
    QString stylesheetCustom;
    std::vector<Filter*> domainBasedHidingFilters = m_filterContainer.getDomainBasedHidingFilters(domain);
    for (Filter *filter : domainBasedHidingFilters)
    {
        QString filterArg = filter->getEvalString();
        stylesheet.append(QString("'%1',").arg(filterArg.replace(QString("'"), QString("\\'"))));
    }

    // Check for custom stylesheet rules
    domainBasedHidingFilters = m_filterContainer.getDomainBasedCustomHidingFilters(domain);
    for (Filter *filter : domainBasedHidingFilters)
    {
        stylesheetCustom.append(filter->getEvalString());
    }

    if (!stylesheet.isEmpty())
    {
        stylesheet = stylesheet.left(stylesheet.size() - 1);
        stylesheet = styleScriptAlt.arg(stylesheet);
    }
    if (!stylesheetCustom.isEmpty())
    {
        stylesheetCustom = stylesheetCustom.replace(QLatin1String("'"), QLatin1String("\\'"));
        stylesheetCustom = styleScript.arg(stylesheetCustom);

        stylesheet.append(QString("\n%1").arg(stylesheetCustom));
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

    QString domain = url.host().toLower();
    QString requestUrl = url.toString(QUrl::FullyEncoded).toLower();

    if (domain.isEmpty())
    {
        domain = requestUrl.mid(requestUrl.indexOf(QStringLiteral("://")) + 3);
        if (domain.contains(QChar('/')))
            domain = domain.left(domain.indexOf(QChar('/')));
    }

    // Check for cache hit
    std::string requestHostStdStr = url.host().toLower().toStdString();
    if (requestHostStdStr.empty())
        requestHostStdStr = domain.toStdString();

    if (m_jsInjectionCache.has(requestHostStdStr))
        return m_jsInjectionCache.get(requestHostStdStr);

    QString scriptlets;
    QString proceduralFilters;
    std::vector<QString> cspDirectives;

    std::vector<Filter*> domainBasedScripts = m_filterContainer.getDomainBasedScriptInjectionFilters(domain);
    for (Filter *filter : domainBasedScripts)
        scriptlets.append(filter->getEvalString());

    std::vector<Filter*> cosmeticProceduralFilters = m_filterContainer.getDomainBasedCosmeticProceduralFilters(domain);
    for (Filter *filter : cosmeticProceduralFilters)
        proceduralFilters.append(filter->getEvalString());

    const Filter *inlineScriptBlockingRule = m_filterContainer.findInlineScriptBlockingFilter(requestUrl, domain);
    if (inlineScriptBlockingRule != nullptr)
        cspDirectives.push_back(QLatin1String("script-src 'unsafe-eval' * blob: data:"));

    std::vector<Filter*> cspFilters = m_filterContainer.getMatchingCSPFilters(requestUrl, domain);
    for (Filter *filter : cspFilters)
        cspDirectives.push_back(filter->getContentSecurityPolicy());

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
        scriptlets.append(cspScript.arg(cspConcatenated));
    }

    if (!scriptlets.isEmpty() || !proceduralFilters.isEmpty())
    {
        result = m_cosmeticJSTemplate;
        result.replace(QStringLiteral("{{ADBLOCK_INTERNAL_SCRIPTLET}}"), scriptlets);
        result.replace(QStringLiteral("{{ADBLOCK_INTERNAL_COSMETIC}}"), proceduralFilters);
    }

    m_jsInjectionCache.put(requestHostStdStr, result);
    return m_jsInjectionCache.get(requestHostStdStr);
}

bool AdBlockManager::shouldBlockRequest(QWebEngineUrlRequestInfo &info, const QUrl &firstPartyUrl)
{
    if (!m_enabled || SchemeRegistry::isSchemeWhitelisted(info.requestUrl().scheme().toLower()))
        return false;

    return m_requestHandler->shouldBlockRequest(info, firstPartyUrl);
}

quint64 AdBlockManager::getRequestsBlockedCount() const
{
    return m_requestHandler->getTotalNumberOfBlockedRequests();
}

int AdBlockManager::getNumberAdsBlocked(const QUrl &url) const
{
    return m_requestHandler->getNumberAdsBlocked(url);
}

QString AdBlockManager::getResource(const QString &key) const
{
    QString keyNoSuffix = key;
    keyNoSuffix = keyNoSuffix.replace(QRegularExpression("(\\.[a-zA-Z]+)$"), QString());
    const bool hasKey = m_resourceMap.contains(key);

    QString resource;

    if (!hasKey && !m_resourceMap.contains(keyNoSuffix))
        resource = m_resourceMap.value(getResourceFromAlias(key));

    if (resource.isEmpty())
        resource = hasKey ? m_resourceMap.value(key) : m_resourceMap.value(keyNoSuffix);

    return resource;
}

QString AdBlockManager::getResourceFromAlias(const QString &alias) const
{
    return m_resourceAliasMap.value(alias);
}

QString AdBlockManager::getResourceContentType(const QString &key) const
{
    if (m_resourceContentTypeMap.contains(key))
        return m_resourceContentTypeMap.value(key);
    if (m_resourceAliasMap.contains(key))
        return m_resourceContentTypeMap.value(m_resourceAliasMap.value(key));
    return QString();
}

int AdBlockManager::getNumSubscriptions() const
{
    return static_cast<int>(m_subscriptions.size());
}

const Subscription *AdBlockManager::getSubscription(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return nullptr;

    return &m_subscriptions.at(static_cast<std::size_t>(index));
}

void AdBlockManager::toggleSubscriptionEnabled(int index)
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return;

    Subscription &sub = m_subscriptions.at(static_cast<std::size_t>(index));
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
    QDir resourceDir(QString("%1%2%3").arg(m_subscriptionDir, QDir::separator(), QStringLiteral("resources")));
    if (!resourceDir.exists())
        resourceDir.mkpath(QStringLiteral("."));

    // Iterate through files in directory, loading into m_resourceMap
    QDirIterator resourceItr(resourceDir.absolutePath(), QDir::Files);
    while (resourceItr.hasNext())
    {
        loadResourceFile(resourceItr.next());
    }

    // Load built-in resources as well
    loadResourceFile(QStringLiteral(":/AdBlockResources.txt"));

    // Finally, load resource aliases
    loadResourceAliases();
}

void AdBlockManager::loadResourceAliases()
{
    QFile f(QStringLiteral(":/AdBlockAliases.txt"));
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;

    QString line;
    QTextStream stream(&f);
    while (stream.readLineInto(&line))
    {
        line = line.trimmed();

        int delimIdx = line.indexOf(QChar('='));
        if (line.isEmpty() || delimIdx < 1)
            continue;

        const QString alias = line.left(delimIdx);
        const QString resourceName = line.mid(delimIdx + 1);
        m_resourceAliasMap.insert(alias, resourceName);
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

void AdBlockManager::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::AdBlockPlusEnabled)
    {
        setEnabled(value.toBool());
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
            m_requestHandler->setTotalNumberOfBlockedRequests(it.value().toString().toULongLong());
            continue;
        }
        Subscription subscription(key);

        subscriptionObj = it.value().toObject();
        subscription.setEnabled(subscriptionObj.value(QLatin1String("enabled")).toBool());

        // Get last update as unix epoch value
        bool ok;
        qint64 lastUpdateRaw = subscriptionObj.value(QLatin1String("last_update")).toVariant().toLongLong(&ok);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        QDateTime lastUpdate = (ok && lastUpdateRaw > 0 ? QDateTime::fromSecsSinceEpoch(lastUpdateRaw) : QDateTime::currentDateTime());
#else
        QDateTime lastUpdate = (ok && lastUpdateRaw > 0 ? QDateTime::fromMSecsSinceEpoch(lastUpdateRaw * 1000LL) : QDateTime::currentDateTime());
#endif
        subscription.setLastUpdate(lastUpdate);

        // Attempt to get next update time as unix epoch value
        qint64 nextUpdateRaw = subscriptionObj.value(QLatin1String("next_update")).toVariant().toLongLong(&ok);
        if (ok && nextUpdateRaw > 0)
        {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
            QDateTime nextUpdate = QDateTime::fromSecsSinceEpoch(nextUpdateRaw);
#else
            QDateTime nextUpdate = QDateTime::fromMSecsSinceEpoch(nextUpdateRaw * 1000LL);
#endif
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
    m_filterContainer.clearFilters();
}

void AdBlockManager::extractFilters()
{
    for (Subscription &s : m_subscriptions)
    {
        // calling load() does nothing if subscription is disabled
        s.load(this);
    }

    m_filterContainer.extractFilters(m_subscriptions);
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
    configObj.insert(QLatin1String("requests_blocked"), QJsonValue(QString::number(m_requestHandler->getTotalNumberOfBlockedRequests())));
    for (auto it = m_subscriptions.cbegin(); it != m_subscriptions.cend(); ++it)
    {
        QJsonObject subscriptionObj;
        subscriptionObj.insert(QLatin1String("enabled"), it->isEnabled());
#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
        subscriptionObj.insert(QLatin1String("last_update"), QJsonValue::fromVariant(QVariant(it->getLastUpdate().toSecsSinceEpoch())));
        subscriptionObj.insert(QLatin1String("next_update"), QJsonValue::fromVariant(QVariant(it->getNextUpdate().toSecsSinceEpoch())));
#else
        subscriptionObj.insert(QLatin1String("last_update"), QJsonValue::fromVariant(QVariant(it->getLastUpdate().toMSecsSinceEpoch() / 1000ULL)));
        subscriptionObj.insert(QLatin1String("next_update"), QJsonValue::fromVariant(QVariant(it->getNextUpdate().toMSecsSinceEpoch() / 1000ULL)));
#endif
        subscriptionObj.insert(QLatin1String("source"), it->getSourceUrl().toString(QUrl::FullyEncoded));

        configObj.insert(it->getFilePath(), QJsonValue(subscriptionObj));
    }

    QJsonDocument configDoc(configObj);
    configFile.write(configDoc.toJson());
    configFile.close();
}

}
