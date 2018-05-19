#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"
#include "Settings.h"
#include "UserScriptManager.h"
#include "WebPage.h"

#include <QFile>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <iostream>

WebPage::WebPage(QObject *parent) :
    QWebEnginePage(parent),
    m_mainFrameHost(),
    m_domainFilterStyle()
{
    // Add frame event handlers for script injection
    connect(this, &WebPage::loadFinished, this, &WebPage::onLoadFinished);
    connect(this, &WebPage::loadStarted, this, &WebPage::onLoadStarted);
    connect(this, &WebPage::urlChanged, this, &WebPage::onMainFrameUrlChanged);
}

WebPage::WebPage(QWebEngineProfile *profile, QObject *parent) :
    QWebEnginePage(profile, parent),
    m_mainFrameHost(),
    m_domainFilterStyle()
{
    // Add frame event handlers for script injection
    connect(this, &WebPage::loadFinished, this, &WebPage::onLoadFinished);
    connect(this, &WebPage::loadStarted, this, &WebPage::onLoadStarted);
    connect(this, &WebPage::urlChanged, this, &WebPage::onMainFrameUrlChanged);
}

void WebPage::javaScriptConsoleMessage(WebPage::JavaScriptConsoleMessageLevel /*level*/, const QString &message, int lineId, const QString &sourceId)
{
    std::cout << "[JS Console] [Source " << sourceId.toStdString() << "] Line " << lineId << ", message: " << message.toStdString() << std::endl; 
}

/*void WebPage::onUnsupportedContent(QNetworkReply *reply)
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
*/

void WebPage::onMainFrameUrlChanged(const QUrl &url)
{
    QString urlHost = url.host().toLower();
    if (!urlHost.isEmpty() && m_mainFrameHost.compare(urlHost) != 0)
    {
        m_mainFrameHost = urlHost;
        m_domainFilterStyle = QString("<style>%1</style>").arg(AdBlockManager::instance().getDomainStylesheet(url));
        m_domainFilterStyle.replace("'", "\\'");
    }
}

void WebPage::onLoadStarted()
{
    injectUserJavaScript(ScriptInjectionTime::DocumentStart);
}

void WebPage::onLoadFinished(bool ok)
{
    if (!ok)
        return;

    injectUserJavaScript(ScriptInjectionTime::DocumentEnd);

    QUrl pageUrl = url();

    QString adBlockStylesheet = AdBlockManager::instance().getStylesheet(pageUrl);
    adBlockStylesheet.replace("'", "\\'");
    runJavaScript(QString("document.body.insertAdjacentHTML('beforeend', '%1');").arg(adBlockStylesheet));
    runJavaScript(QString("document.body.insertAdjacentHTML('beforeend', '%1');").arg(m_domainFilterStyle));

    QString adBlockJS = AdBlockManager::instance().getDomainJavaScript(pageUrl);
    if (!adBlockJS.isEmpty())
        runJavaScript(adBlockJS);
}

void WebPage::injectUserJavaScript(ScriptInjectionTime injectionTime)
{
    // Attempt to get URL associated with frame
    QUrl pageUrl = url();
    if (pageUrl.isEmpty())
        return;

    QString userJS = sBrowserApplication->getUserScriptManager()->getScriptsFor(pageUrl, injectionTime, true);
    if (!userJS.isEmpty())
        runJavaScript(userJS);
}
