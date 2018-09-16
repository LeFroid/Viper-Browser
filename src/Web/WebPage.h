#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "UserScript.h"

#include <vector>

#include <QString>
#include <QWebEnginePage>
#include <QtWebEngineCoreVersion>

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
#include <QWebEngineQuotaRequest>
#include <QWebEngineRegisterProtocolHandlerRequest>
#endif

class QWebEngineProfile;

/**
 * @class WebPage
 * @brief Wrapper class for QT's QWebPage, used in order to
 *        handle cookies properly
 */
class WebPage : public QWebEnginePage
{
    friend class WebView;

    Q_OBJECT

public:
    /// WebPage constructor
    WebPage(QObject *parent = nullptr);

    /// Constructs the web page with a specific browsing profile and a parent
    WebPage(QWebEngineProfile *profile, QObject *parent = nullptr);

    /// Executes the given JavaScript code (in string form), storing the result in the given reference
    void runJavaScriptNonBlocking(const QString &scriptSource, QVariant &result);

    /// Executes the given JavaScript code (string form), and waits until the result can be returned
    QVariant runJavaScriptBlocking(const QString &scriptSource);

signals:
    /// Emitted when a print request is made from the web page
    void printPageRequest();

protected:
    /// Called upon receiving a request to navigate to the specified url by means of the given navigation type. If the method returns true, the request is accepted
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;

    /// Called when a JavaScript program attempts to print the given message to the browser console
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineId, const QString &sourceId) override;

    /// Called when an invalid certificate error is raised while loading a given request. Returns true when ignoring the error, or false when the page will not be loaded.
    bool certificateError(const QWebEngineCertificateError &certificateError) override;

private slots:
    /// Opens an authentication dialog when requested by the given URL
    void onAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator);

    /// Opens a proxy authentication dialog when requested by the given URL for the proxy host
    void onProxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator, const QString &proxyHost);

    /// Handles the feature permission request signal
    void onFeaturePermissionRequested(const QUrl &securityOrigin, Feature feature);

    /// Called when the URL of the main frame has changed
    void onMainFrameUrlChanged(const QUrl &url);

    /// Called when the page has loaded by the given percent amount
    void onLoadProgress(int percent);

    /// Called when a frame is finished loading
    void onLoadFinished(bool ok);

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    /// Called when a page requests larger persistent storage than the application's current allocation in File System API.
    void onQuotaRequested(QWebEngineQuotaRequest quotaRequest);

    /// Called when a page tries to register a custom protocol using the registerProtocolHandler API
    void onRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request);
#endif

    /// Handler for render process termination
    void onRenderProcessTerminated(RenderProcessTerminationStatus terminationStatus, int exitCode);

    /// Shows the render / tab crash page
    void showTabCrashedPage();

    /// Injects any javascript into the page, if applicable
    void injectUserJavaScript(ScriptInjectionTime injectionTime);

private:
    /// Connects web engine page signals to their handlers
    void setupSlots();

private:
    /// Stores the host of the main frame's URL. Used to prevent excessive requests to fetch the AdBlockManager's domain-specific cosmetic filters
    QString m_mainFrameHost;

    /// Stores the current page's domain-specific cosmetic filters in string form
    QString m_domainFilterStyle;

    /// Scripts injected by ad block during load progress and load finish
    QString m_mainFrameAdBlockScript;

    /// True if ad block script needs to be injected in the page during load time, false if else
    bool m_needInjectAdBlockScript;
};

#endif // WEBPAGE_H
