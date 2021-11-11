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
#include "RequestInterceptor.h"
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
#include <QWebEngineSettings>
#include <QWebEngineScript>
#include <QWebEngineScriptCollection>
#include <QtWebEngineCoreVersion>

#include <algorithm>
#include <iostream>
#include <iterator>

WebPage::WebPage(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QWebEnginePage(parent),
    m_adBlockManager(serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager")),
    m_userScriptManager(serviceLocator.getServiceAs<UserScriptManager>("UserScriptManager")),
    m_history(new WebHistory(serviceLocator, this)),
    m_originalUrl(),
    m_mainFrameAdBlockScript(),
    m_injectedAdblock(false),
    m_permissionsAllowed(),
    m_permissionsDenied()
{
    QTimer::singleShot(0, this, [this, &serviceLocator](){
        setupSlots(serviceLocator);
    });
}

WebPage::WebPage(const ViperServiceLocator &serviceLocator, QWebEngineProfile *profile, QObject *parent) :
    QWebEnginePage(profile, parent),
    m_adBlockManager(serviceLocator.getServiceAs<adblock::AdBlockManager>("AdBlockManager")),
    m_userScriptManager(serviceLocator.getServiceAs<UserScriptManager>("UserScriptManager")),
    m_history(new WebHistory(serviceLocator, this)),
    m_originalUrl(),
    m_mainFrameAdBlockScript(),
    m_injectedAdblock(false),
    m_permissionsAllowed(),
    m_permissionsDenied()
{
    QTimer::singleShot(0, this, [this, &serviceLocator](){
        setupSlots(serviceLocator);
    });
}

void WebPage::setupSlots(const ViperServiceLocator &serviceLocator)
{
    setUrlRequestInterceptor(new RequestInterceptor(serviceLocator, this));

    AutoFill *autoFillManager = serviceLocator.getServiceAs<AutoFill>("AutoFill");

    QWebChannel *channel = new QWebChannel(this);
    channel->registerObject(QLatin1String("extStorage"), serviceLocator.getServiceAs<ExtStorage>("storage"));
    channel->registerObject(QLatin1String("favoritePageManager"), serviceLocator.getServiceAs<FavoritePagesManager>("favoritePageManager"));
    channel->registerObject(QLatin1String("autofill"), new AutoFillBridge(autoFillManager, this));
    channel->registerObject(QLatin1String("favicons"), new FaviconStoreBridge(serviceLocator.getServiceAs<FaviconManager>("FaviconManager"), this));
    setWebChannel(channel, QWebEngineScript::ApplicationWorld);

    connect(this, &WebPage::authenticationRequired,      this, &WebPage::onAuthenticationRequired);
    connect(this, &WebPage::certificateError,            this, &WebPage::onCertificateError);
    connect(this, &WebPage::proxyAuthenticationRequired, this, &WebPage::onProxyAuthenticationRequired);
    connect(this, &WebPage::loadFinished,                this, &WebPage::onLoadFinished);
    connect(this, &WebPage::loadFinished,     autoFillManager, &AutoFill::onPageLoaded);
    connect(this, &WebPage::featurePermissionRequested,  this, &WebPage::onFeaturePermissionRequested);
    connect(this, &WebPage::renderProcessTerminated,     this, &WebPage::onRenderProcessTerminated);

    connect(this, &WebPage::loadProgress, this, [this](int progress) {
        if (!m_injectedAdblock && progress >= 22 && progress < 100)
        {
            m_injectedAdblock = true;
            if (!m_mainFrameAdBlockScript.isEmpty())
                runJavaScript(m_mainFrameAdBlockScript, QWebEngineScript::UserWorld);
        }
    });

    connect(this, &WebPage::quotaRequested, this, &WebPage::onQuotaRequested);
    connect(this, &WebPage::registerProtocolHandlerRequested, this, &WebPage::onRegisterProtocolHandlerRequested);
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
            Q_EMIT printPageRequest();
            return false;
        }
    }

    if (!isMainFrame)
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);

    m_injectedAdblock = false;
    m_originalUrl = QUrl();

    const QWebEngineSettings *pageSettings = settings();
    if (!pageSettings->testAttribute(QWebEngineSettings::PluginsEnabled)
            || !pageSettings->testAttribute(QWebEngineSettings::PdfViewerEnabled))
    {
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
                QByteArray bytes = data.toUtf8();
                setHtml(bytes, url);
                return false;
            }
        }
    }

    if (type == QWebEnginePage::NavigationTypeLinkClicked)
    {
        // If only change in URL is fragment, try to update URL bar by emitting url changed signal
        if (this->url().toString(QUrl::RemoveFragment).compare(url.toString(QUrl::RemoveFragment)) == 0)
            Q_EMIT urlChanged(url);
    }

    if (type != QWebEnginePage::NavigationTypeReload)
    {
        URL pageUrl(url);
        m_mainFrameAdBlockScript = m_adBlockManager->getDomainJavaScript(pageUrl);

        QWebEngineScriptCollection &scriptCollection = scripts();
        scriptCollection.clear();
        auto pageScripts = m_userScriptManager->getAllScriptsFor(url);
        for (auto &script : pageScripts)
            scriptCollection.insert(script);

        if (!m_mainFrameAdBlockScript.isEmpty())
        {
            QWebEngineScript adBlockScript;
            adBlockScript.setSourceCode(m_mainFrameAdBlockScript);
            adBlockScript.setName(QLatin1String("viper-content-blocker-userworld"));
            adBlockScript.setRunsOnSubFrames(true);
            adBlockScript.setWorldId(QWebEngineScript::UserWorld);
            adBlockScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
            scriptCollection.insert(adBlockScript);

            // Inject into the DOM as a script tag
            m_mainFrameAdBlockScript.replace(QString("\\"), QString("\\\\"));
            m_mainFrameAdBlockScript.replace(QString("${"), QString("\\${"));
            const static QString mutationScript = QStringLiteral("function selfInject() { "
                                             "try { let script = document.createElement('script'); "
                                             "script.appendChild(document.createTextNode(`%1`)); "
                                             "if (document.head || document.documentElement) { (document.head || document.documentElement).appendChild(script); } "
                                             "else { setTimeout(selfInject, 100); } "
                                             " } catch(exc) { console.error('Could not run mutation script: ' + exc); } } selfInject();");
            m_mainFrameAdBlockScript = mutationScript.arg(m_mainFrameAdBlockScript);
        }

        const QString domainFilterStyle = m_adBlockManager->getDomainStylesheet(pageUrl);
        if (!domainFilterStyle.isEmpty())
        {
            QWebEngineScript adBlockCosmeticScript;
            adBlockCosmeticScript.setRunsOnSubFrames(true);
            adBlockCosmeticScript.setSourceCode(domainFilterStyle);
            adBlockCosmeticScript.setName(QLatin1String("viper-cosmetic-blocker"));
            adBlockCosmeticScript.setWorldId(QWebEngineScript::UserWorld);
            adBlockCosmeticScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
            scriptCollection.insert(adBlockCosmeticScript);
        }

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

    QTimer::singleShot(1000, loop.data(), [&loop](){
        if (!loop.isNull() && loop->isRunning())
            loop->quit();
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
    // level = info, warning or error
    //TODO: create class named WebPageConsoleLog that stores this info. Can then tie the logs into a console widget, similar to the ad block log
    QWebEnginePage::javaScriptConsoleMessage(level, message, lineId, sourceId);
    //std::cout << "[JS Console] [Source " << sourceId.toStdString() << "] Line " << lineId << ", message: " << message.toStdString() << std::endl;
}

void WebPage::onCertificateError(const QWebEngineCertificateError &certificateError)
{
    SecurityManager::instance().onCertificateError(certificateError, qobject_cast<QWidget*>(parent())->window());
}

QWebEnginePage *WebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    WebView *webView = qobject_cast<WebView*>(parent());
    const bool isPrivate = profile()->objectName().compare(QStringLiteral("PrivateWebProfile")) == 0;

    switch (type)
    {
        case QWebEnginePage::WebBrowserWindow:    // Open a new window
        {
            MainWindow *win = isPrivate ? sBrowserApplication->getNewPrivateWindow() : sBrowserApplication->getNewWindow();
            return win->currentWebWidget()->page();
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
    }
    return nullptr;
}

void WebPage::onAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator)
{
    WebView *webView = qobject_cast<WebView*>(parent());
    if (!webView)
        return;

    AuthDialog authDialog(webView->window());
    authDialog.setMessage(tr("%1 is requesting your username and password for %2")
                          .arg(requestUrl.host(), authenticator->realm()));

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
    WebView *webView = qobject_cast<WebView*>(parent());
    if (!webView)
        return;

    AuthDialog authDialog(webView->window());
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

bool WebPage::isPermissionAllowed(const QUrl &securityOrigin, WebPage::Feature feature) const
{
    if (!m_permissionsAllowed.contains(securityOrigin))
        return false;

    const auto features = m_permissionsAllowed.value(securityOrigin);
    return std::find(std::begin(features), std::end(features), feature) != std::end(features);
}

bool WebPage::isPermissionDenied(const QUrl &securityOrigin, WebPage::Feature feature) const
{
    if (!m_permissionsDenied.contains(securityOrigin))
        return false;

    const auto features = m_permissionsDenied.value(securityOrigin);
    return std::find(std::begin(features), std::end(features), feature) != std::end(features);
}

void WebPage::onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
    WebView *webView = qobject_cast<WebView*>(parent());
    if (!webView)
        return;

    if (isPermissionAllowed(securityOrigin, feature))
    {
        setFeaturePermission(securityOrigin, feature, PermissionGrantedByUser);
        return;
    }
    else if (isPermissionDenied(securityOrigin, feature))
    {
        setFeaturePermission(securityOrigin, feature, PermissionDeniedByUser);
        return;
    }

    QString featureStr = tr("Allow %1 to ");
    switch (feature)
    {
        case WebPage::Geolocation:
            featureStr.append(tr("access your location?"));
            break;
        case WebPage::MediaAudioCapture:
            featureStr.append(tr("access your microphone?"));
            break;
        case WebPage::MediaVideoCapture:
            featureStr.append(tr("access your webcam?"));
            break;
        case WebPage::MediaAudioVideoCapture:
            featureStr.append(tr("access your microphone and webcam?"));
            break;
        case WebPage::MouseLock:
            featureStr.append(tr("lock your mouse?"));
            break;
        case WebPage::DesktopVideoCapture:
            featureStr.append(tr("capture video of your desktop?"));
            break;
        case WebPage::DesktopAudioVideoCapture:
            featureStr.append(tr("capture audio and video of your desktop?"));
            break;
        default:
            setFeaturePermission(securityOrigin, feature, PermissionDeniedByUser);
            return;
    }

    QString requestStr = featureStr.arg(securityOrigin.host());
    QMessageBox::StandardButton choice = QMessageBox::question(webView->window(), tr("Web Permission Request"), requestStr, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    PermissionPolicy policy = (choice == QMessageBox::Yes ? PermissionGrantedByUser : PermissionDeniedByUser);
    setFeaturePermission(securityOrigin, feature, policy);

    // Save the decision in memory
    QHash<QUrl, std::vector<WebPage::Feature>> *permissionStore 
        = policy == PermissionGrantedByUser ? &m_permissionsAllowed : &m_permissionsDenied;
    if (permissionStore->contains(securityOrigin))
    {
        std::vector<WebPage::Feature> &featureSet = (*permissionStore)[securityOrigin];
        featureSet.push_back(feature);
    }
    else
    {
        std::vector<WebPage::Feature> featureSet{feature};
        permissionStore->insert(securityOrigin, featureSet);
    }
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

    if (!m_mainFrameAdBlockScript.isEmpty())
        runJavaScript(m_mainFrameAdBlockScript, QWebEngineScript::ApplicationWorld);
}

void WebPage::onQuotaRequested(QWebEngineQuotaRequest quotaRequest)
{
    WebView *webView = qobject_cast<WebView*>(parent());
    if (!webView)
        return;

    auto response = QMessageBox::question(webView->window(), tr("Permission Request"),
                                          tr("Allow %1 to increase its storage quota to %2?")
                                          .arg(quotaRequest.origin().host(), CommonUtil::bytesToUserFriendlyStr(quotaRequest.requestedSize())));
    if (response == QMessageBox::Yes)
        quotaRequest.accept();
    else
        quotaRequest.reject();
}

void WebPage::onRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request)
{
    WebView *webView = qobject_cast<WebView*>(parent());
    if (!webView)
        return;

    //todo: save user response, add a "Site Preferences" UI where site permissions and such can be modified
    auto response = QMessageBox::question(webView->window(), tr("Permission Request"),
                                          tr("Allow %1 to open all %2 links?").arg(request.origin().host(), request.scheme()));
    if (response == QMessageBox::Yes)
        request.accept();
    else
        request.reject();
}

void WebPage::onRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus terminationStatus, int exitCode)
{
    if (terminationStatus == WebPage::NormalTerminationStatus)
        return;

    qDebug() << "Render process terminated with status " << static_cast<int>(terminationStatus)
             << ", exit code " << exitCode;

    QTimer::singleShot(50, this, &WebPage::showTabCrashedPage);
}

void WebPage::showTabCrashedPage()
{
    QFile resource(QStringLiteral(":/crash"));
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
