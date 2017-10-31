#include "BrowserApplication.h"
#include "NetworkAccessManager.h"
#include "WebPage.h"

QString WebPage::m_userAgent = QString();

WebPage::WebPage(QObject *parent) :
    QWebPage(parent)
{
    setNetworkAccessManager(sBrowserApplication->getNetworkAccessManager());

    // for this signal - create and then render a "Not found" html page
    //connect(this, &WebPage::unsupportedContent, this, &WebPage::onUnsupportedContent);
}

void WebPage::enablePrivateBrowsing()
{
    setNetworkAccessManager(sBrowserApplication->getPrivateNetworkAccessManager());
}

void WebPage::setUserAgent(const QString &userAgent)
{
    m_userAgent = userAgent;
}

QString WebPage::userAgentForUrl(const QUrl &url) const
{
    if (!m_userAgent.isNull())
        return m_userAgent;

    return QWebPage::userAgentForUrl(url);
}
