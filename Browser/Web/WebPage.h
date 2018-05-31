#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "UserScript.h"

#include <future>
#include <QString>
#include <QWebEnginePage>

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

protected:
    /// Called upon receiving a request to navigate to the specified url by means of the given navigation type. If the method returns true, the request is accepted
    bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override;

    /// Executes the given JavaScript code (in string form), storing the result in the given reference
    void runJavaScriptNonBlocking(const QString &scriptSource, QVariant &result);

    /// Called when a JavaScript program attempts to print the given message to the browser console
    void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineId, const QString &sourceId);

private slots:
    /// Attempts to handle unsupported network replies
//    void onUnsupportedContent(QNetworkReply *reply);

    /// Called when the URL of the main frame has changed
    void onMainFrameUrlChanged(const QUrl &url);

    /// Called when a frame has started loading
    void onLoadStarted();

    /// Called when a frame is finished loading
    void onLoadFinished(bool ok);

    /// Injects any javascript into the page, if applicable
    void injectUserJavaScript(ScriptInjectionTime injectionTime);

private:
    /// Stores the host of the main frame's URL. Used to prevent excessive requests to fetch the AdBlockManager's domain-specific cosmetic filters
    QString m_mainFrameHost;

    /// Stores the current page's domain-specific cosmetic filters in string form
    QString m_domainFilterStyle;

    /// Stores the last URL to be accepted or rejected by acceptNavigationRequest(...)
    QUrl m_lastUrl;
};

#endif // WEBPAGE_H
