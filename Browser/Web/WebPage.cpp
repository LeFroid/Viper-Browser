#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "NetworkAccessManager.h"
#include "Settings.h"
#include "WebPage.h"

#include <QFile>
#include <QWebElement>
#include <QWebFrame>
#include <QWebSecurityOrigin>
#include <QDebug>

QString WebPage::m_userAgent = QString();

WebPage::WebPage(QObject *parent) :
    QWebPage(parent)
{
    setNetworkAccessManager(sBrowserApplication->getNetworkAccessManager());
    setForwardUnsupportedContent(true);

    connect(this, &WebPage::unsupportedContent, this, &WebPage::onUnsupportedContent);
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
