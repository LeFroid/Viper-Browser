#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include <map>
#include <memory>
#include <vector>
#include <QDateTime>
#include <QNetworkCookieJar>
#include <QUrl>
#include <QWebEngineCookieStore>

/**
 * @class CookieJar
 * @brief Implements ability to store and load cookies on disk
 */
class CookieJar : public QNetworkCookieJar
{
    friend class CookieTableModel;

    Q_OBJECT

public:
    /// Constructs the cookie jar
    explicit CookieJar(bool enableCookies, bool privateJar = false, QObject *parent =nullptr);

    /// Saves cookies to database before calling ~QNetworkCookieJar
    ~CookieJar();

    /// Returns true if the CookieJar contains 1 or more cookies associated with the given host, false if else
    bool hasCookiesFor(const QString &host) const;

    /// Erases all cookies from the jar
    void eraseAllCookies();

    /// Enables cookies to be stored if the given value is true, otherwise all cookies will be deleted when the cookieAdded
    /// signal is emitted by the cookie store
    void setCookiesEnabled(bool value);

    /// Enables third party cookies to be set if the given value is true, otherwise will filter out third party cookies that aren't exempt
    void setThirdPartyCookiesEnabled(bool value);

signals:
    /// Emitted when a new cookie has been added to the jar
    void cookieAdded();

    /// Emitted when all the cookies have been erased
    void cookiesRemoved();

private slots:
    /// Called when a cookie has been added to the cookie store
    void onCookieAdded(const QNetworkCookie &cookie);

private:
    /// Removes expired cookies from both the database and the list in memory
    void removeExpired();

private:
    /// True if cookies are enabled by the user, false if all cookies will immediately be removed
    bool m_enableCookies;

    /// True if private browsing cookie jar (e.g., no persistence), false if standard cookie jar
    bool m_privateJar;

    /// Pointer to the web engine's cookie store
    QWebEngineCookieStore *m_store;

    /// Vector of exempt third party cookie setters
    std::vector<std::string> m_exemptParties; //TODO: implement persistent storage of exempt parties
};

#endif // COOKIEJAR_H
