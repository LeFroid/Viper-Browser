#include "AdBlockManager.h"
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

namespace AdBlock
{
    AdBlockManager::AdBlockManager(QObject *parent) :
        QObject(parent),
        m_enabled(true),
        m_subscriptionDir(),
        m_stylesheet(),
        m_subscriptions(),
        m_blockFilters(),
        m_allowFilters(),
        m_domainStyleFilters()
    {
        m_enabled = sBrowserApplication->getSettings()->getValue("AdBlockPlusEnabled").toBool();
        loadSubscriptions();
    }

    AdBlockManager::~AdBlockManager()
    {
    }

    AdBlockManager &AdBlockManager::instance()
    {
        static AdBlockManager adBlockInstance;
        return adBlockInstance;
    }

    const QString &AdBlockManager::getStylesheet() const
    {
        return m_stylesheet;
    }

    QString AdBlockManager::getDomainStylesheet(const QString &domain) const
    {
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

        return stylesheet;
    }

    BlockedNetworkReply *AdBlockManager::getBlockedReply(const QNetworkRequest &request)
    {
        QUrl requestUrlObj = request.url();

        QString baseUrl;
        QWebFrame *frame = qobject_cast<QWebFrame*>(request.originatingObject());
        if (frame != nullptr)
        {
            while (frame->parentFrame() != nullptr)
                frame = frame->parentFrame();
            baseUrl = frame->baseUrl().toString(QUrl::FullyEncoded);
        }
        else
            baseUrl = requestUrlObj.toString(QUrl::FullyEncoded);

        QString requestUrl = requestUrlObj.toString(QUrl::FullyEncoded).toLower();
        QString requestDomain = requestUrlObj.host().toLower();

        if (baseUrl.isEmpty())
            baseUrl = requestUrl;

        ElementType elemType = getElementTypeMask(request, requestUrlObj.path());
        // Document type and third party type checking are done outside of getElementTypeMask
        if (requestUrl.compare(baseUrl) == 0)
            elemType |= ElementType::Document;
        if (getSecondLevelDomain(requestUrlObj).compare(getSecondLevelDomain(QUrl(baseUrl))) != 0)
            elemType |= ElementType::ThirdParty;

        // Compare to filters
        for (AdBlockFilter *filter : m_allowFilters)
        {
            if (filter->isMatch(baseUrl, requestUrl, requestDomain, elemType))
            {
                qDebug() << "Exception rule match. BaseURL: " << baseUrl << " request URL: " << requestUrl << " request domain: " << requestDomain << " rule: " << filter->getRule();
                return nullptr;
            }
        }
        for (AdBlockFilter *filter : m_blockFilters)
        {
            if (filter->isMatch(baseUrl, requestUrl, requestDomain, elemType))
            {
                qDebug() << "matched block rule  BaseURL: " << baseUrl << " request URL: " << requestUrl << " request domain: " << requestDomain << " rule: " << filter->getRule();
                return new BlockedNetworkReply(request, this);
            }
        }

        return nullptr;
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

    QString AdBlockManager::getSecondLevelDomain(const QUrl &url)
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

    void AdBlockManager::loadSubscriptions()
    {
        if (!m_enabled)
            return;

        std::shared_ptr<Settings> settings = sBrowserApplication->getSettings();

        m_subscriptionDir = settings->getPathValue("AdBlockPlusDataDir");
        QDir subscriptionDir(m_subscriptionDir);
        if (!subscriptionDir.exists())
            subscriptionDir.mkpath(m_subscriptionDir);

        QFile configFile(settings->getPathValue("AdBlockPlusConfig"));
        if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
            return;

        // Attempt to parse config/subscription info file
        QByteArray configData = configFile.readAll();
        configFile.close();

        // Add subscriptions to container
        QJsonDocument configDoc(QJsonDocument::fromJson(configData));
        QJsonObject configObj = configDoc.object();
        for (auto it = configObj.begin(); it != configObj.end(); ++it)
        {
            AdBlockSubscription subscription(it.key());
            bool enabled = it.value().toBool(false);
            subscription.setEnabled(enabled);
            m_subscriptions.push_back(std::move(subscription));
        }

        // Used to store css rules for the global stylesheet and domain-specific stylesheets
        QHash<QString, AdBlockFilter*> stylesheetFilterMap;
        QHash<QString, AdBlockFilter*> stylesheetExceptionMap;

        // Setup global stylesheet string
        m_stylesheet = QStringLiteral("<style>");

        for (AdBlockSubscription &s : m_subscriptions)
        {
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
                else
                {
                    if (filter->isException())
                        m_allowFilters.push_back(filter);
                    else
                        m_blockFilters.push_back(filter);
                }
            }
        }

        QHashIterator<QString, AdBlockFilter*> it(stylesheetExceptionMap);
        while (it.hasNext())
        {
            it.next();
            if (!stylesheetFilterMap.contains(it.key()))
                continue;

            AdBlockFilter *filter = it.value();
            stylesheetFilterMap.value(it.key())->m_domainWhitelist.unite(filter->m_domainBlacklist);
        }

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
}
