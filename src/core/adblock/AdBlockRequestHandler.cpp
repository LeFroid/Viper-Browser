#include "AdBlockFilter.h"
#include "AdBlockLog.h"
#include "AdBlockManager.h"
#include "AdBlockRequestHandler.h"
#include "URL.h"

#include <QDateTime>
#include <QUrl>

namespace adblock
{

RequestHandler::RequestHandler(FilterContainer &filterContainer, AdBlockLog *log, QObject *parent) :
    QObject(parent),
    m_filterContainer(filterContainer),
    m_log(log),
    m_numRequestsBlocked(0),
    m_pageAdBlockCount()
{
}

void RequestHandler::loadStarted(const QUrl &url)
{
    m_pageAdBlockCount[url] = 0;
}

int RequestHandler::getNumberAdsBlocked(const QUrl &url) const
{
    auto it = m_pageAdBlockCount.find(url);
    if (it != m_pageAdBlockCount.end())
        return *it;

    return 0;
}

quint64 RequestHandler::getTotalNumberOfBlockedRequests() const
{
    return m_numRequestsBlocked;
}

void RequestHandler::setTotalNumberOfBlockedRequests(quint64 count)
{
    m_numRequestsBlocked = count;
}

bool RequestHandler::shouldBlockRequest(QWebEngineUrlRequestInfo &info, const QUrl &firstPartyUrl)
{
    // Get request URL and the originating URL
    const QUrl requestUrl = info.requestUrl();
    const QString requestUrlStr = info.requestUrl().toString(QUrl::FullyEncoded).toLower();

    const URL requestUrlWrapper { requestUrl };

    // Get hostname of the first party URL (main frame)
    const QString baseUrl = firstPartyUrl.host().toLower();

    // Hostname of the request URL
    QString domain = requestUrl.host().toLower();
    if (domain.isEmpty())
    {
        domain = requestUrlStr.mid(requestUrlStr.indexOf(QStringLiteral("://")) + 3).toLower();
        if (domain.contains(QChar('/')))
            domain = domain.left(domain.indexOf(QChar('/')));
    }

    // Convert QWebEngine request type to AdBlockFilter request type
    ElementType elemType = getRequestType(info, firstPartyUrl);

    // Compare to filters
    Filter *matchingBlockFilter = m_filterContainer.findImportantBlockingFilter(baseUrl, requestUrlStr, domain, elemType);
    if (matchingBlockFilter != nullptr)
    {
        ++m_numRequestsBlocked;
        ++m_pageAdBlockCount[firstPartyUrl];

        if (matchingBlockFilter->isRedirect())
        {
            info.redirect(QUrl(QString("blocked:%1").arg(matchingBlockFilter->getRedirectName())));
            m_log->addEntry(FilterAction::Redirect, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
            return false;
        }

        m_log->addEntry(FilterAction::Block, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
        return true;
    }

    matchingBlockFilter = m_filterContainer.findBlockingRequestFilter(requestUrlWrapper.getSecondLevelDomain(), baseUrl, requestUrlStr, domain, elemType);

    // Stop here if we did not find a blocking filter - let the request proceed
    if (matchingBlockFilter == nullptr)
        return false;

    if (Filter *filter = m_filterContainer.findWhitelistingFilter(baseUrl, requestUrlStr, domain, elemType))
    {
        m_log->addEntry(FilterAction::Allow, firstPartyUrl, requestUrl, elemType, filter->getRule(), QDateTime::currentDateTime());
        return false;
    }

    // If we reach this point, then the matching block filter is applied to the request
    ++m_numRequestsBlocked;
    ++m_pageAdBlockCount[firstPartyUrl];

    if (matchingBlockFilter->isRedirect())
    {
        info.redirect(QUrl(QString("blocked:%1").arg(matchingBlockFilter->getRedirectName())));
        m_log->addEntry(FilterAction::Redirect, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
        return false;
    }

    m_log->addEntry(FilterAction::Block, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
    return true;
}

ElementType RequestHandler::getRequestType(const QWebEngineUrlRequestInfo &info, const QUrl &firstPartyUrl) const
{
    const URL firstPartyUrlWrapper { firstPartyUrl };
    const URL requestUrl { info.requestUrl() };
    const QString requestUrlStr = requestUrl.toString(QUrl::FullyEncoded).toLower();

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

    // Check for websocket
    // Doesn't seem to work though. If only we could check for the presence of
    // request headers such as Sec-WebSocket-Key or Sec-WebSocket-Version, then
    // we could detect websocket requests..
    const QString requestScheme = requestUrl.scheme();
    if (requestScheme.compare(QStringLiteral("ws")) == 0 || requestScheme.compare(QStringLiteral("wss")) == 0)
        elemType |= ElementType::WebSocket;

    // Check for third party request type
    if (firstPartyUrlWrapper.isEmpty()
            || (firstPartyUrlWrapper.toString().compare(QLatin1String(".")) == 0)
            || (firstPartyUrlWrapper.toString().compare(QLatin1String("data;,")) == 0)
            || (requestUrl.getSecondLevelDomain() != firstPartyUrlWrapper.getSecondLevelDomain()))
        elemType |= ElementType::ThirdParty;

    return elemType;
}

}
