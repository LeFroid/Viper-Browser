#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "ServiceLocator.h"
#include "UserScript.h"

#include <utility>
#include <vector>

#include <QHash>
#include <QString>
#include <QWebEnginePage>
#include <QtWebEngineCoreVersion>
#include <QWebEngineQuotaRequest>
#include <QWebEngineRegisterProtocolHandlerRequest>

namespace adblock {
    class AdBlockManager;
}

class UserScriptManager;
class WebHistory;

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
    WebPage(const ViperServiceLocator &serviceLocator, QObject *parent = nullptr);

    /// Constructs the web page with a specific browsing profile and a parent
    WebPage(const ViperServiceLocator &serviceLocator, QWebEngineProfile *profile, QObject *parent = nullptr);

    /// WebPage destructor
    ~WebPage() = default;

    /// Returns a pointer to the web page's history
    WebHistory *getHistory() const;

    /// Returns the URL associated with the page after a navigation request was accepted, but before it was fully loaded.
    const QUrl &getOriginalUrl() const;

    /// Executes the given JavaScript code (in string form), storing the result in the given reference
    void runJavaScriptNonBlocking(const QString &scriptSource, QVariant &result);

    /// Executes the given JavaScript code (string form), and waits until the result can be returned
    QVariant runJavaScriptBlocking(const QString &scriptSource);

Q_SIGNALS:
    /// Emitted when a print request is made from the web page
    void printPageRequest();

protected:
    /// Called upon receiving a request to navigate to the specified url by means of the given navigation type. If the method returns true, the request is accepted
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;

    /// Called when a JavaScript program attempts to print the given message to the browser console
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineId, const QString &sourceId) override;

    /// Creates a new web page in a new window, new tab or as a dialog, depending on the given type, and returns a pointer to the page
    QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type) override;

private Q_SLOTS:
    /// Called when an invalid certificate error is raised while loading a given request. Returns true when ignoring the error, or false when the page will not be loaded.
    void onCertificateError(const QWebEngineCertificateError &certificateError);

    /// Opens an authentication dialog when requested by the given URL
    void onAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator);

    /// Opens a proxy authentication dialog when requested by the given URL for the proxy host
    void onProxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *authenticator, const QString &proxyHost);

    /// Handles the feature permission request signal
    void onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature);

    /// Called when a frame is finished loading
    void onLoadFinished(bool ok);

    /// Called when a page requests larger persistent storage than the application's current allocation in File System API.
    void onQuotaRequested(QWebEngineQuotaRequest quotaRequest);

    /// Called when a page tries to register a custom protocol using the registerProtocolHandler API
    void onRegisterProtocolHandlerRequested(QWebEngineRegisterProtocolHandlerRequest request);

    /// Handler for render process termination
    void onRenderProcessTerminated(QWebEnginePage::RenderProcessTerminationStatus terminationStatus, int exitCode);

    /// Shows the render / tab crash page
    void showTabCrashedPage();

    /// Injects any javascript into the page, if applicable
    void injectUserJavaScript(ScriptInjectionTime injectionTime);

private:
    /// Connects web engine page signals to their handlers
    void setupSlots(const ViperServiceLocator &serviceLocator);

    /// Returns true if the web feature is permitted for the given origin, false if not explicitly
    /// allowed (does not imply that a permission has been denied).
    bool isPermissionAllowed(const QUrl &securityOrigin, WebPage::Feature feature) const;
    
    /// Returns true if the web feature has been denied by the user for the given origin, or false
    /// if not explicitly blocked by the user. If isPermissionAllowed(...) and isPermissionDenied(...)
    /// both return false, the user will be prompted to choose whether to allow or block the feature.
    bool isPermissionDenied(const QUrl &securityOrigin, WebPage::Feature feature) const;

private:
    /// Advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

    /// User script system manager
    UserScriptManager *m_userScriptManager;

    /// Stores the history of the web page
    WebHistory *m_history;

    /// Contains the original URL of the current page, as passed to the WebPage in the acceptNavigationRequest method
    QUrl m_originalUrl;

    /// Scripts injected by ad block during load progress and load finish
    QString m_mainFrameAdBlockScript;

    /// Flag indicating whether or not we need to inject the adblock script into the DOM
    bool m_injectedAdblock;

    /// Stores feature permissions that were allowed by the user, in the current session, local to this page.
    /// TODO: Later we can extend this to persistent storage, across web pages, through a permission manager,
    ///       but only if the user "opts-in" to it.
    QHash<QUrl, std::vector<WebPage::Feature>> m_permissionsAllowed;

    /// Stores feature permissions that were denied by the user, in the current session, local to this page.
    QHash<QUrl, std::vector<WebPage::Feature>> m_permissionsDenied;
};

#endif // WEBPAGE_H
