#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "ExtStorage.h"
#include "SecurityManager.h"
#include "Settings.h"
#include "URL.h"
#include "UserScriptManager.h"
#include "WebPage.h"

#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include <QWebChannel>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QtWebEngineCoreVersion>

#include <iostream>

WebPage::WebPage(QObject *parent) :
    QWebEnginePage(parent),
    m_mainFrameHost(),
    m_domainFilterStyle(),
    m_mainFrameAdBlockScript(),
    m_needInjectAdBlockScript(true)
{
    setupSlots();
}

WebPage::WebPage(QWebEngineProfile *profile, QObject *parent) :
    QWebEnginePage(profile, parent),
    m_mainFrameHost(),
    m_domainFilterStyle(),
    m_needInjectAdBlockScript(true)
{
    setupSlots();
}

void WebPage::setupSlots()
{
    //QWebChannel *channel = new QWebChannel(this);
    //channel->registerObject(QLatin1String("extStorage"), sBrowserApplication->getExtStorage());
    //setWebChannel(channel);

    connect(this, &WebPage::loadProgress,               this, &WebPage::onLoadProgress);
    connect(this, &WebPage::loadFinished,               this, &WebPage::onLoadFinished);
    connect(this, &WebPage::urlChanged,                 this, &WebPage::onMainFrameUrlChanged);
    connect(this, &WebPage::featurePermissionRequested, this, &WebPage::onFeaturePermissionRequested);
    connect(this, &WebPage::renderProcessTerminated,    this, &WebPage::onRenderProcessTerminated);
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

    if (!QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame))
        return false;

    if (isMainFrame && type != QWebEnginePage::NavigationTypeReload)
    {
        QWebEngineScriptCollection &scriptCollection = scripts();
        scriptCollection.clear();
        auto pageScripts = sBrowserApplication->getUserScriptManager()->getAllScriptsFor(url);
        for (auto &script : pageScripts)
            scriptCollection.insert(script);
    }

    return true;
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

void WebPage::onFeaturePermissionRequested(const QUrl &securityOrigin, WebPage::Feature feature)
{
    QString featureStr = tr("Allow %1 to ");
    switch (feature)
    {
        case WebPage::Geolocation:
            featureStr.append("access your location?");
            break;
        case WebPage::MediaAudioCapture:
            featureStr.append("access your microphone?");
            break;
        case WebPage::MediaVideoCapture:
            featureStr = tr("access your webcam?");
            break;
        case WebPage::MediaAudioVideoCapture:
            featureStr = tr("access your microphone and webcam?");
            break;
        case WebPage::MouseLock:
            featureStr = tr("lock your mouse?");
            break;
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 10, 0))
        case WebPage::DesktopVideoCapture:
            featureStr = tr("capture video of your desktop?");
            break;
        case WebPage::DesktopAudioVideoCapture:
            featureStr = tr("capture audio and video of your desktop?");
            break;
#endif
        default:
            featureStr = QString();
            break;
    }

    // Deny unknown features
    if (featureStr.isEmpty())
    {
        setFeaturePermission(securityOrigin, feature, PermissionDeniedByUser);
        return;
    }

    QString requestStr = featureStr.arg(securityOrigin.host());
    auto choice = QMessageBox::question(view()->window(), tr("Web Permission Request"), requestStr);
    PermissionPolicy policy = choice == QMessageBox::Yes ? PermissionGrantedByUser : PermissionDeniedByUser;
    setFeaturePermission(securityOrigin, feature, policy);
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

void WebPage::onLoadProgress(int percent)
{
    if (percent > 0 && percent < 100 && m_needInjectAdBlockScript)
    {
        m_needInjectAdBlockScript = false;

        URL pageUrl(url());
        m_mainFrameAdBlockScript = AdBlockManager::instance().getDomainJavaScript(pageUrl);
        if (!m_mainFrameAdBlockScript.isEmpty())
            runJavaScript(m_mainFrameAdBlockScript);
    }

    if (percent == 100)
        emit loadFinished(true);
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

    if (m_needInjectAdBlockScript)
        m_mainFrameAdBlockScript = AdBlockManager::instance().getDomainJavaScript(pageUrl);

    if (!m_mainFrameAdBlockScript.isEmpty())
        runJavaScript(m_mainFrameAdBlockScript);

    m_needInjectAdBlockScript = true;
}

void WebPage::onRenderProcessTerminated(RenderProcessTerminationStatus terminationStatus, int exitCode)
{
    if (terminationStatus == WebPage::NormalTerminationStatus)
        return;

    qDebug() << "Render process terminated with status " << static_cast<int>(terminationStatus)
             << ", exit code " << exitCode;

    QTimer::singleShot(50, this, &WebPage::showTabCrashedPage);
}

void WebPage::showTabCrashedPage()
{
    QFile resource(":/crash");
    if (resource.open(QIODevice::ReadOnly))
    {
        QString pageHtml = QString::fromUtf8(resource.readAll().constData()).arg(title());
        setHtml(pageHtml, url());
    }
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
