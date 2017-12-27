#include "AdBlockManager.h"
#include "AdBlockModel.h"
#include "Bitfield.h"
#include "BrowserApplication.h"
#include "Settings.h"
#include "WebPage.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QWebFrame>

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
    m_allowFilters(),
    m_domainStyleFilters(),
    m_domainJSFilters(),
    m_customStyleFilters(),
    m_adBlockModel(nullptr)
{
    m_enabled = sBrowserApplication->getSettings()->getValue("AdBlockPlusEnabled").toBool();
    loadSubscriptions();
    loadDynamicTemplate();
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

AdBlockModel *AdBlockManager::getModel()
{
    if (m_adBlockModel == nullptr)
        m_adBlockModel = new AdBlockModel(this);

    return m_adBlockModel;
}

const QString &AdBlockManager::getStylesheet() const
{
    return m_stylesheet;
}

QString AdBlockManager::getDomainStylesheet(const QUrl &url) const
{
    QString domain = getSecondLevelDomain(url);
    if (domain.isEmpty())
        return QString();

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

    return stylesheet;
}

QString AdBlockManager::getDomainJavaScript(const QUrl &url) const
{
    QString domain = getSecondLevelDomain(url);
    if (domain.isEmpty())
        domain = url.host();

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
        {
            //qDebug() << "Exception rule match. BaseURL: " << baseUrl << " request URL: " << requestUrl << " request domain: " << requestDomain << " rule: " << filter->getRule();
            return nullptr;
        }
    }
    for (AdBlockFilter *filter : m_blockFilters)
    {
        if (filter->isMatch(baseUrl, requestUrl, secondLevelDomain, elemType))
        {
            //qDebug() << "Matched block rule  BaseURL: " << baseUrl << " request URL: " << requestUrl << " request domain: " << requestDomain << " rule: " << filter->getRule();
            return new BlockedNetworkReply(request, filter->getRule(), this);
        }
    }

    return nullptr;
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
    QFile templateFile(":/AdBlock.js");
    if (!templateFile.open(QIODevice::ReadOnly))
        return;

    m_cosmeticJSTemplate = templateFile.readAll();
    templateFile.close();
}

void AdBlockManager::loadSubscriptions()
{
    if (!m_enabled)
        return;

    std::shared_ptr<Settings> settings = sBrowserApplication->getSettings();

    m_subscriptionDir = settings->getPathValue("AdBlockPlusDataDir");
    QDir subscriptionDir(m_subscriptionDir);
    if (!subscriptionDir.exists())
        subscriptionDir.mkpath(m_subscriptionDir);

    m_configFile = settings->getPathValue("AdBlockPlusConfig");
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
        QDateTime nextUpdate;
        if (ok && nextUpdateUInt > 0)
            nextUpdate = QDateTime::fromTime_t(nextUpdateUInt);
        subscription.setNextUpdate(nextUpdate);

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
}

void AdBlockManager::extractFilters()
{
    // Used to store css rules for the global stylesheet and domain-specific stylesheets
    QHash<QString, AdBlockFilter*> stylesheetFilterMap;
    QHash<QString, AdBlockFilter*> stylesheetExceptionMap;

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
            else
            {
                if (filter->isException())
                    m_allowFilters.push_back(filter);
                else if (filter->isImportant())
                    m_importantBlockFilters.push_back(filter);
                else
                    m_blockFilters.push_back(filter);
            }
        }
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
    m_stylesheet.append("</style>");
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
