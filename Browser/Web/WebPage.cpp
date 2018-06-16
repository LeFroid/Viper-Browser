#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "SecurityManager.h"
#include "Settings.h"
#include "URL.h"
#include "UserScriptManager.h"
#include "WebPage.h"

#include <QPointer>
#include <QTimer>

#include <QFile>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <iostream>

WebPage::WebPage(QObject *parent) :
    QWebEnginePage(parent),
    m_mainFrameHost(),
    m_domainFilterStyle()
{
    setupSlots();
}

WebPage::WebPage(QWebEngineProfile *profile, QObject *parent) :
    QWebEnginePage(profile, parent),
    m_mainFrameHost(),
    m_domainFilterStyle()
{
    setupSlots();
}

void WebPage::setupSlots()
{
    // Add frame event handlers for script injection
    connect(this, &WebPage::loadFinished, this, &WebPage::onLoadFinished);
    connect(this, &WebPage::urlChanged, this, &WebPage::onMainFrameUrlChanged);
}

bool WebPage::acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame)
{
    // Check if special url such as "viper:print"
    if (url.scheme().compare(QLatin1String("viper")) == 0)
    {
        if (url.path().compare(QLatin1String("print")) == 0)
        {
            emit printPageRequest();
            return false;
        }
    }

    // Check if the request is for a PDF and try to render with PDF.js
    const QString urlString = url.toString(QUrl::FullyEncoded);
    if (urlString.endsWith(QLatin1String(".pdf")))
    {
        QFile resource(":/viewer.html");
        bool opened = resource.open(QIODevice::ReadOnly);
        if (opened)
        {
            QString data = QString::fromUtf8(resource.readAll().constData());
            int endTag = data.indexOf("</html>");
            data.insert(endTag - 1, QString("<script>document.addEventListener(\"DOMContentLoaded\", function() {{"
                                            "PDFJS.verbosity = PDFJS.VERBOSITY_LEVELS.info;"
                                            "window.PDFViewerApplication.open(\"%1\");}});</script>").arg(urlString));
            QByteArray bytes;
            bytes.append(data);
            setHtml(bytes, url);
            return false;
        }
    }

    if (isMainFrame && type != QWebEnginePage::NavigationTypeReload)
    {
        QWebEngineScriptCollection &scriptCollection = scripts();
        scriptCollection.clear();
        auto pageScripts = sBrowserApplication->getUserScriptManager()->getAllScriptsFor(url);
        for (auto &script : pageScripts)
            scriptCollection.insert(script);
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

void WebPage::runJavaScriptNonBlocking(const QString &scriptSource, QVariant &result)
{
    runJavaScript(scriptSource, [&](const QVariant &returnValue) {
        result.setValue(returnValue);
    });
}

QVariant WebPage::runJavaScriptBlocking(const QString &scriptSource)
{
    QEventLoop loop;
    QVariant result;

    runJavaScript(scriptSource, [&](const QVariant &returnValue){
        result = returnValue;
        loop.quit();
    });

    connect(this, &WebPage::destroyed, &loop, &QEventLoop::quit);

    loop.exec();
    return result;
}

void WebPage::javaScriptConsoleMessage(WebPage::JavaScriptConsoleMessageLevel level, const QString &message, int lineId, const QString &sourceId)
{
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineId, sourceId);
    //std::cout << "[JS Console] [Source " << sourceId.toStdString() << "] Line " << lineId << ", message: " << message.toStdString() << std::endl;
}

bool WebPage::certificateError(const QWebEngineCertificateError &certificateError)
{
    return SecurityManager::instance().onCertificateError(certificateError);
}

void WebPage::onMainFrameUrlChanged(const QUrl &url)
{
    URL urlCopy(url);
    QString urlHost = urlCopy.host().toLower();
    if (!urlHost.isEmpty() && m_mainFrameHost.compare(urlHost) != 0)
    {
        m_mainFrameHost = urlHost;
        m_domainFilterStyle = QString("<style>%1</style>").arg(AdBlockManager::instance().getDomainStylesheet(urlCopy));
        m_domainFilterStyle.replace("'", "\\'");
    }
}

void WebPage::onLoadFinished(bool ok)
{
    if (!ok)
        return;

    URL pageUrl(url());

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
