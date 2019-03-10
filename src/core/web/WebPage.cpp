#include "AdBlockManager.h"
#include "AuthDialog.h"
#include "AutoFill.h"
#include "AutoFillBridge.h"
#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "CommonUtil.h"
#include "ExtStorage.h"
#include "FaviconManager.h"
#include "FaviconStoreBridge.h"
#include "FavoritePagesManager.h"
#include "MainWindow.h"
#include "SecurityManager.h"
#include "Settings.h"
#include "URL.h"
#include "UserScriptManager.h"
#include "WebDialog.h"
#include "WebHistory.h"
#include "WebPage.h"
#include "WebView.h"

#include <QAuthenticator>
#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include <QWebChannel>
#include <QWebEngineProfile>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QtWebEngineCoreVersion>

#include <iostream>

WebPage::WebPage(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QWebEnginePage(parent),
    m_adBlockManager(serviceLocator.getServiceAs<AdBlockManager>("AdBlockManager")),
    m_userScriptManager(serviceLocator.getServiceAs<UserScriptManager>("UserScriptManager")),
    m_history(new WebHistory(serviceLocator, this)),
    m_originalUrl(),
    m_mainFrameHost(),
    m_domainFilterStyle(),
    m_mainFrameAdBlockScript(),
    m_needInjectAdBlockScript(true)
{
    setupSlots(serviceLocator);
}

WebPage::WebPage(const ViperServiceLocator &serviceLocator, QWebEngineProfile *profile, QObject *parent) :
    QWebEnginePage(profile, parent),
    m_adBlockManager(serviceLocator.getServiceAs<AdBlockManager>("AdBlockManager")),
    m_userScriptManager(serviceLocator.getServiceAs<UserScriptManager>("UserScriptManager")),
    m_history(new WebHistory(serviceLocator, this)),
    m_originalUrl(),
    m_mainFrameHost(),
    m_domainFilterStyle(),
    m_needInjectAdBlockScript(true)
{
    setupSlots(serviceLocator);
}

WebPage::~WebPage()
{
}

void WebPage::setupSlots(const ViperServiceLocator &serviceLocator)
{
    AutoFill *autoFillManager = serviceLocator.getServiceAs<AutoFill>("AutoFill");

    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QLatin1String("extStorage"), serviceLocator.getServiceAs<ExtStorage>("storage"));
    channel->registerObject(QLatin1String("favoritePageManager"), serviceLocator.getServiceAs<FavoritePagesManager>("favoritePageManager"));
    channel->registerObject(QLatin1String("autofill"), new AutoFillBridge(autoFillManager, this));
    channel->registerObject(QLatin1String("favicons"), new FaviconStoreBridge(serviceLocator.getServiceAs<FaviconManager>("FaviconManager"), this));
    setWebChannel(channel, QWebEngineScript::ApplicationWorld);

    connect(this, &WebPage::authenticationRequired,      this, &WebPage::onAuthenticationRequired);
    connect(this, &WebPage::proxyAuthenticationRequired, this, &WebPage::onProxyAuthenticationRequired);
    connect(this, &WebPage::loadProgress,                this, &WebPage::onLoadProgress);
    connect(this, &WebPage::loadFinished,                this, &WebPage::onLoadFinished);
    connect(this, &WebPage::loadFinished,     autoFillManager, &AutoFill::onPageLoaded);
    connect(this, &WebPage::urlChanged,                  this, &WebPage::onMainFrameUrlChanged);
    connect(this, &WebPage::featurePermissionRequested,  this, &WebPage::onFeaturePermissionRequested);
    connect(this, &WebPage::renderProcessTerminated,     this, &WebPage::onRenderProcessTerminated);

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    connect(this, &WebPage::quotaRequested, this, &WebPage::onQuotaRequested);
    connect(this, &WebPage::registerProtocolHandlerRequested, this, &WebPage::onRegisterProtocolHandlerRequested);
#endif
}

WebHistory *WebPage::getHistory() const
{
    return m_history;
}

const QUrl &WebPage::getOriginalUrl() const
{
    return m_originalUrl;
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

    if (!isMainFrame)
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);

    m_originalUrl = QUrl();

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
            data.insert(endTag - 1, QString("<script>window.Response = undefined; document.addEventListener(\"DOMContentLoaded\", function() {"
                                            " window.PDFViewerApplicationOptions.set('verbosity', pdfjsLib.VerbosityLevel.INFOS); "
                                            " window.PDFViewerApplication.open(\"%1\");});</script>").arg(urlString));
            QByteArray bytes;
            bytes.append(data);
            setHtml(bytes, url);
            return false;
        }
    }

    if (type == QWebEnginePage::NavigationTypeLinkClicked)
    {
        // If only change in URL is fragment, try to update URL bar by emitting url changed signal
        if (this->url().toString(QUrl::RemoveFragment).compare(url.toString(QUrl::RemoveFragment)) == 0)
            emit urlChanged(url);
    }

    if (type != QWebEnginePage::NavigationTypeReload)
    {
        QWebEngineScriptCollection &scriptCollection = scripts();
        scriptCollection.clear();
        auto pageScripts = m_userScriptManager->getAllScriptsFor(url);
        for (auto &script : pageScripts)
            scriptCollection.insert(script);

        if (type != QWebEnginePage::NavigationTypeBackForward)
            m_originalUrl = url;
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
    QPointer<QEventLoop> loop = new QEventLoop();
    QVariant result;

    runJavaScript(scriptSource, [&result, loop](const QVariant &returnValue){
        if (!loop.isNull() && loop->isRunning())
        {
            result = returnValue;
            loop->quit();
        }
    });

    connect(this, &WebPage::renderProcessTerminated, this, [&loop](RenderProcessTerminationStatus terminationStatus, int /*exitCode*/){
        if (!loop.isNull() && terminationStatus != WebPage::NormalTerminationStatus)
            loop->quit();
    });
    connect(this, &WebPage::destroyed, loop.data(), &QEventLoop::quit);

    loop->exec(QEventLoop::ExcludeUserInputEvents);
    delete loop;

    return result;
}

void WebPage::javaScriptConsoleMessage(WebPage::JavaScriptConsoleMessageLevel level, const QString &message, int lineId, const QString &sourceId)
{
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineId, sourceId);
    //std::cout << "[JS Console] [Source " << sourceId.toStdString() << "] Line " << lineId << ", message: " << message.toStdString() << std::endl;
}

bool WebPage::certificateError(const QWebEngineCertificateError &certificateError)
{
    return SecurityManager::instance().onCertificateError(certificateError, view()->window());
}

QWebEnginePage *WebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    WebView *webView = qobject_cast<WebView*>(view());
    const bool isPrivate = profile() != QWebEngineProfile::defaultProfile();

    switch (type)
    {
        case QWebEnginePage::WebBrowserWindow:    // Open a new window
        {
            MainWindow *win = isPrivate ? sBrowserApplication->getNewPrivateWindow() : sBrowserApplication->getNewWindow();
            return win->getTabWidget()->currentWebWidget()->page();
        }
        case QWebEnginePage::WebBrowserBackgroundTab:
        case QWebEnginePage::WebBrowserTab:
        {
            // Get main window
            MainWindow *win = qobject_cast<MainWindow*>(webView->window());
            if (!win)
            {
                QObject *obj = webView->parent();
                while (obj->parent() != nullptr)
                    obj = obj->parent();

                win = qobject_cast<MainWindow*>(obj);

                if (!win)
                    return nullptr;
            }

            const bool switchToNewTab = (type == QWebEnginePage::WebBrowserTab
                                         && !sBrowserApplication->getSettings()->getValue(BrowserSetting::OpenAllTabsInBackground).toBool());

            WebWidget *webWidget = switchToNewTab ? win->getTabWidget()->newTab() : win->getTabWidget()->newBackgroundTab();
            return webWidget->page();
        }
        case QWebEnginePage::WebDialog:     // Open a web dialog
        {
            class WebDialog *dialog = new class WebDialog(isPrivate);
            dialog->show();
            return dialog->getView()->page();
        }
        default: break;
    }
    return nullptr;
}

void WebPage::onAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator)
{
    AuthDialog authDialog(view()->window());
    authDialog.setMessage(tr("%1 is requesting your username and password for %2")
                          .arg(requestUrl.host()).arg(authenticator->realm()));

    if (authDialog.exec() == QDialog::Accepted)
    {
        authenticator->setUser(authDialog.getUsername());
        authenticator->setPassword(authDialog.getPassword());
    }
    else
    {
        *authenticator = QAuthenticator();
    }
}

void WebPage::onProxyAuthenticationRequired(const QUrl &/*requestUrl*/, QAuthenticator *authenticator, const QString &proxyHost)
{
    AuthDialog authDialog(view()->window());
    authDialog.setMessage(tr("Authentication is required to connect to proxy %1").arg(proxyHost));

    if (authDialog.exec() == QDialog::Accepted)
    {
        authenticator->setUser(authDialog.getUsername());
        authenticator->setPassword(authDialog.getPassword());
    }
    else
    {
        *authenticator = QAuthenticator();
    }
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
        m_domainFilterStyle = QString("<style>%1</style>").arg(m_adBlockManager->getDomainStylesheet(urlCopy));
        m_domainFilterStyle.replace("'", "\\'");
    }
}

void WebPage::onLoadProgress(int percent)
{
    if (percent > 0 && percent < 100 && m_needInjectAdBlockScript)
    {
        URL pageUrl(url());
        if (pageUrl.host().isEmpty())
            return;

        m_needInjectAdBlockScript = false;

        m_mainFrameAdBlockScript = m_adBlockManager->getDomainJavaScript(pageUrl);
        if (!m_mainFrameAdBlockScript.isEmpty())
            runJavaScript(m_mainFrameAdBlockScript, QWebEngineScript::ApplicationWorld);
    }

    //if (percent == 100)
    //    emit loadFinished(true);
}

void WebPage::onLoadFinished(bool ok)
{
    if (!ok)
        return;

    if (!m_originalUrl.isEmpty())
        m_originalUrl = requestedUrl();

    URL pageUrl(url());

    QString adBlockStylesheet = m_adBlockManager->getStylesheet(pageUrl);
    adBlockStylesheet.replace("'", "\\'");
    runJavaScript(QString("document.body.insertAdjacentHTML('beforeend', '%1');").arg(adBlockStylesheet));
    runJavaScript(QString("document.body.insertAdjacentHTML('beforeend', '%1');").arg(m_domainFilterStyle));

    if (m_needInjectAdBlockScript)
        m_mainFrameAdBlockScript = m_adBlockManager->getDomainJavaScript(pageUrl);

    if (!m_mainFrameAdBlockScript.isEmpty())
        runJavaScript(m_mainFrameAdBlockScript, QWebEngineScript::UserWorld);

    m_needInjectAdBlockScript = true;
}

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
void WebPage::onQuotaRequested(QWebEngineQuotaRequest quotaRequest)
{
    auto response = QMessageBox::question(view()->window(), tr("Permission Request"),
                                          tr("Allow %1 to increase its storage quota to %2?")
                                          .arg(quotaRequest.origin().host())
                                          .arg(CommonUtil::bytesToUserFriendlyStr(quotaRequest.requestedSize())));
    if (response == QMessageBox::Yes)
        quotaRequest.accept();
    else
        quotaRequest.reject();
}

void WebPage::onRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request)
{
    //todo: save user response, add a "Site Preferences" UI where site permissions and such can be modified
    auto response = QMessageBox::question(view()->window(), tr("Permission Request"),
                                          tr("Allow %1 to open all %2 links?").arg(request.origin().host()).arg(request.scheme()));
    if (response == QMessageBox::Yes)
        request.accept();
    else
        request.reject();
}
#endif

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

// Was used when backend was QtWebEngine. No longer used. Will keep this method
// for now in case it may be needed in the future.
void WebPage::injectUserJavaScript(ScriptInjectionTime injectionTime)
{
    // Attempt to get URL associated with frame
    const QUrl pageUrl = url();
    if (pageUrl.isEmpty())
        return;

    const QString userJS = m_userScriptManager->getScriptsFor(pageUrl, injectionTime, true);
    if (!userJS.isEmpty())
        runJavaScript(userJS);
}
