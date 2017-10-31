#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <QString>
#include <QWebPage>

/**
 * @class WebPage
 * @brief Wrapper class for QT's QWebPage, used in order to
 *        handle cookies properly
 */
class WebPage : public QWebPage
{
    Q_OBJECT

public:
    /// WebPage constructor
    WebPage(QObject *parent = 0);

    /// Sets the network access manager to the browser's private browsing one
    void enablePrivateBrowsing();

    /// Sets the user agent to be sent with all requests
    static void setUserAgent(const QString &userAgent);

protected:
    /// Returns the user agent for a given url. If no user agent has been manually specified, the default agent will be sent
    QString userAgentForUrl(const QUrl &url) const override;

protected:
    /// Custom user agent string. Empty if default UA is used
    static QString m_userAgent;
};

#endif // WEBPAGE_H
