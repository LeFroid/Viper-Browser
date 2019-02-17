#include "AdBlockFilter.h"
#include "AdBlockLog.h"
#include "AdBlockManager.h"
#include "AdBlockRequestHandler.h"
#include "URL.h"

#include <QDateTime>
#include <QUrl>

AdBlockRequestHandler::AdBlockRequestHandler(AdBlockFilterContainer &filterContainer, AdBlockLog *log, QObject *parent) :
    QObject(parent),
    m_filterContainer(filterContainer),
    m_log(log),
    m_numRequestsBlocked(0),
    m_pageAdBlockCount()
{
}

void AdBlockRequestHandler::loadStarted(const QUrl &url)
{
    m_pageAdBlockCount[url] = 0;
}

int AdBlockRequestHandler::getNumberAdsBlocked(const QUrl &url) const
{
    auto it = m_pageAdBlockCount.find(url);
    if (it != m_pageAdBlockCount.end())
        return *it;

    return 0;
}

quint64 AdBlockRequestHandler::getTotalNumberOfBlockedRequests() const
{
    return m_numRequestsBlocked;
}

void AdBlockRequestHandler::setTotalNumberOfBlockedRequests(quint64 count)
{
    m_numRequestsBlocked = count;
}

bool AdBlockRequestHandler::shouldBlockRequest(QWebEngineUrlRequestInfo &info)
{
    // Get request URL and the originating URL
    const QUrl firstPartyUrl = info.firstPartyUrl();
    const QUrl requestUrl = info.requestUrl();
    const QString requestUrlStr = info.requestUrl().toString(QUrl::FullyEncoded).toLower();

    const URL firstPartyUrlWrapper { firstPartyUrl };
    const URL requestUrlWrapper { requestUrl };

    QString baseUrl = firstPartyUrlWrapper.getSecondLevelDomain().toLower();
    if (baseUrl.isEmpty())
        baseUrl = firstPartyUrl.host().toLower();

    // Get request domain
    QString domain = requestUrl.host().toLower();
    if (domain.startsWith(QLatin1String("www.")))
        domain = domain.mid(4);
    if (domain.isEmpty())
        domain = requestUrlWrapper.getSecondLevelDomain();

    // Convert QWebEngine request type to AdBlockFilter request type
    ElementType elemType = getRequestType(info);

    // Compare to filters
    AdBlockFilter *matchingBlockFilter = m_filterContainer.findImportantBlockingFilter(baseUrl, requestUrlStr, domain, elemType);
    if (matchingBlockFilter != nullptr)
    {
        ++m_numRequestsBlocked;
        ++m_pageAdBlockCount[firstPartyUrl];

        if (matchingBlockFilter->isRedirect())
        {
            info.redirect(QUrl(QString("blocked:%1").arg(matchingBlockFilter->getRedirectName())));
            m_log->addEntry(AdBlockFilterAction::Redirect, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
            return false;
        }

        m_log->addEntry(AdBlockFilterAction::Block, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
        return true;
    }

    matchingBlockFilter = m_filterContainer.findBlockingRequestFilter(requestUrlWrapper.getSecondLevelDomain(), baseUrl, requestUrlStr, domain, elemType);

    // Stop here if we did not find a blocking filter - let the request proceed
    if (matchingBlockFilter == nullptr)
        return false;

    if (AdBlockFilter *filter = m_filterContainer.findWhitelistingFilter(baseUrl, requestUrlStr, domain, elemType))
    {
        m_log->addEntry(AdBlockFilterAction::Allow, firstPartyUrl, requestUrl, elemType, filter->getRule(), QDateTime::currentDateTime());
        return false;
    }

    // If we reach this point, then the matching block filter is applied to the request
    ++m_numRequestsBlocked;
    ++m_pageAdBlockCount[firstPartyUrl];

    if (matchingBlockFilter->isRedirect())
    {
        info.redirect(QUrl(QString("blocked:%1").arg(matchingBlockFilter->getRedirectName())));
        m_log->addEntry(AdBlockFilterAction::Redirect, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
        return false;
    }

    m_log->addEntry(AdBlockFilterAction::Block, firstPartyUrl, requestUrl, elemType, matchingBlockFilter->getRule(), QDateTime::currentDateTime());
    return true;
}

ElementType AdBlockRequestHandler::getRequestType(const QWebEngineUrlRequestInfo &info) const
{
    const URL firstPartyUrl { info.firstPartyUrl() };
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

    // Check for third party request type
    if (firstPartyUrl.isEmpty() || (requestUrl.getSecondLevelDomain() != firstPartyUrl.getSecondLevelDomain()))
        elemType |= ElementType::ThirdParty;

    return elemType;
}
