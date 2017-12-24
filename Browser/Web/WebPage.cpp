#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"
#include "Settings.h"
#include "UserScriptManager.h"
#include "WebPage.h"

#include <QFile>
#include <QWebElement>
#include <QWebFrame>
#include <QWebSecurityOrigin>

QString WebPage::m_userAgent = QString();

WebPage::WebPage(QObject *parent) :
    QWebPage(parent),
    m_mainFrameHost(),
    m_domainFilterStyle()
{
    setNetworkAccessManager(sBrowserApplication->getNetworkAccessManager());
    setForwardUnsupportedContent(true);

    connect(this, &WebPage::unsupportedContent, this, &WebPage::onUnsupportedContent);

    // Add frame event handlers for script injection
    QWebFrame *pageMainFrame = mainFrame();
    connect(pageMainFrame, &QWebFrame::loadFinished, this, &WebPage::onLoadFinished);
    connect(pageMainFrame, &QWebFrame::javaScriptWindowObjectCleared, this, &WebPage::onLoadStarted);
    connect(this, &WebPage::frameCreated, [=](QWebFrame *frame) {
        connect(frame, &QWebFrame::loadFinished, this, &WebPage::onLoadFinished);
        connect(frame, &QWebFrame::javaScriptWindowObjectCleared, this, &WebPage::onLoadStarted);
    });
    connect(pageMainFrame, &QWebFrame::urlChanged, this, &WebPage::onMainFrameUrlChanged);
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

void WebPage::onUnsupportedContent(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        reply->deleteLater();
        return;
    }

    // Check if file should be downloaded
    if (reply->hasRawHeader("Content-Disposition"))
    {
        BrowserApplication *app = sBrowserApplication;
        app->getDownloadManager()->handleUnsupportedContent(reply, app->getSettings()->getValue("AskWhereToSaveDownloads").toBool());
        return;
    }

    // Check content type header
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString().toLower();

    // Attempt to render pdf content
    if (contentType.contains("application/pdf"))
    {
        QFile resource(":/viewer.html");
        bool opened = resource.open(QIODevice::ReadOnly);
        if (opened)
        {
            QString data = QString::fromUtf8(resource.readAll().constData());
            int endTag = data.indexOf("</html>");
            data.insert(endTag - 1, QString("<script>document.addEventListener(\"DOMContentLoaded\", function() {{"
                                                "PDFJS.verbosity = PDFJS.VERBOSITY_LEVELS.info;"
                                                "window.PDFViewerApplication.open(\"%1\");}});</script>").arg(reply->url().toString(QUrl::FullyEncoded)));

            QByteArray bytes;
            bytes.append(data);

            // Make sure scheme is not https, PDF.JS will not load properly unless scheme is http
            // Taken from https://github.com/qutebrowser/qutebrowser/issues/2525
            QUrl nonSslUrl(reply->url());
            nonSslUrl.setScheme("http");
            mainFrame()->securityOrigin().addAccessWhitelistEntry(reply->url().scheme(), reply->url().host(), QWebSecurityOrigin::DisallowSubdomains);
            mainFrame()->setContent(bytes, "text/html", nonSslUrl);
        }
    }

    reply->deleteLater();
}

void WebPage::onMainFrameUrlChanged(const QUrl &url)
{
    QString urlHost = url.host().toLower();
    if (!urlHost.isEmpty() && m_mainFrameHost.compare(urlHost) != 0)
    {
        m_mainFrameHost = urlHost;
        m_domainFilterStyle = QString("<style>%1</style>").arg(AdBlockManager::instance().getDomainStylesheet(url));
    }
}

void WebPage::onLoadStarted()
{
    if (QWebFrame *frame = qobject_cast<QWebFrame*>(sender()))
    {
        //bool isMainFrame = (frame == mainFrame());
        //QString extensionJS = m_extensionMgr->getScriptsFor(frame->url(), isMainFrame);
        //if (!extensionJS.isEmpty()) {
        //    frame->addToJavaScriptWindowObject("chrome", m_extensionMgr);
        //    frame->evaluateJavaScript(extensionJS);
        //}
        injectUserJavaScript(frame, ScriptInjectionTime::DocumentStart);
    }
}

void WebPage::onLoadFinished(bool ok)
{
    QWebFrame *frame = qobject_cast<QWebFrame*>(sender());
    if (ok && (frame != nullptr))
        injectUserJavaScript(frame, ScriptInjectionTime::DocumentEnd);

    if (frame == mainFrame())
    {
        frame->documentElement().appendInside(AdBlockManager::instance().getStylesheet());
        frame->documentElement().appendInside(m_domainFilterStyle);
        //frame->documentElement().appendInside(
        //    QString("<style>%1</style>").arg(AdBlockManager::instance().getDomainStylesheet(frame->baseUrl())));
    }
}

void WebPage::injectUserJavaScript(QWebFrame *frame, ScriptInjectionTime injectionTime)
{
    // Attempt to get URL associated with frame
    QUrl url = frame->url();
    if (url.isEmpty())
        url = frame->requestedUrl();

    // If URL still not obtained, do nothing
    if (url.isEmpty())
        return;

    bool isMainFrame = (frame == mainFrame());
    QString userJS = sBrowserApplication->getUserScriptManager()->getScriptsFor(url, injectionTime, isMainFrame);
    if (!userJS.isEmpty())
    {
        frame->evaluateJavaScript(userJS);
    }
}
