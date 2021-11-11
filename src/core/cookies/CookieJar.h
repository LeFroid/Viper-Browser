#ifndef COOKIEJAR_H
#define COOKIEJAR_H

#include "ISettingsObserver.h"
#include "URL.h"

#include <map>
#include <memory>
#include <mutex>
#include <QDateTime>
#include <QNetworkCookieJar>
#include <QSet>
#include <QUrl>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

class Settings;

/**
 * @class CookieJar
 * @brief Implements ability to store and load cookies on disk
 */
class CookieJar : public QNetworkCookieJar, public ISettingsObserver
{
    friend class CookieTableModel;

    Q_OBJECT

public:
    /// Constructs the cookie jar
    explicit CookieJar(Settings *settings, QWebEngineProfile *defaultProfile, bool privateJar = false, QObject *parent =nullptr);

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

    /// Returns a const reference to the set of exempt third party hosts that can set cookies.
    const QSet<URL> &getExemptThirdPartyHosts() const;

Q_SIGNALS:
    /// Emitted when a new cookie has been added to the jar
    void cookieAdded();

    /// Emitted when all the cookies have been erased
    void cookiesRemoved();

public Q_SLOTS:
    /// Adds an exemption for a third party host to be able to store cookies when the third party filter is enabled. Does nothing if filtering is diabled
    void addThirdPartyExemption(const QUrl &hostUrl);

    /// Removes an exemption for a third party host to be able to store cookies when the third party filter is enabled. Does nothing if filtering is diabled
    void removeThirdPartyExemption(const QUrl &hostUrl);

private Q_SLOTS:
    /// Called when a cookie has been added to the cookie store
    void onCookieAdded(const QNetworkCookie &cookie);

    /// Called when a cookie has been removed from the cookie store
    void onCookieRemoved(const QNetworkCookie &cookie);

    /// Listens for any changes to browser settings that may affect the behavior of the cookie jar
    void onSettingChanged(BrowserSetting setting, const QVariant &value) override;

private:
    /// Loads a data file containing all third parties that are exempt to the cookie filter; used if the third party cookie filter is enabled
    void loadExemptThirdParties();

    /// Saves the host names of all third parties that are exempt from the cookie filter to the storage file
    void saveExemptThirdParties();

    /// Removes expired cookies from both the database and the list in memory
    void removeExpired();

private:
    /// True if cookies are enabled by the user, false if all cookies will immediately be removed
    bool m_enableCookies;

    /// True if private browsing cookie jar (e.g., no persistence), false if standard cookie jar
    bool m_privateJar;

    /// Pointer to the web engine's cookie store
    QWebEngineCookieStore *m_store;

    /// Set of exempt third party cookie setters
    QSet<URL> m_exemptParties;

    /// Name of the file containing exceptions to the third-party cookie filtering policy (if enabled)
    QString m_exemptThirdPartyCookieFileName;

    /// Mutex used within the handlers for a cookie being added or removed
    mutable std::mutex m_mutex;
};

#endif // COOKIEJAR_H
