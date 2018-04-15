#ifndef WEBPAGE_H
#define WEBPAGE_H

#include "UserScript.h"
#include <QString>
#include <QWebEnginePage>

/**
 * @class WebPage
 * @brief Wrapper class for QT's QWebPage, used in order to
 *        handle cookies properly
 */
class WebPage : public QWebEnginePage
{
    Q_OBJECT

public:
    /// WebPage constructor
    WebPage(QObject *parent = 0);

    /// Sets the network access manager to the browser's private browsing one
    void enablePrivateBrowsing();

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
};

#endif // WEBPAGE_H
