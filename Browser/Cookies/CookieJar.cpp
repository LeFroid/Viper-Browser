#include <QDateTime>
#include <QFile>
#include <QNetworkCookie>
#include <QWebEngineProfile>

#include <QtGlobal>
#include <QtWebEngineCoreVersion>

#include <QFileInfo>
#include <QDebug>

#include "BrowserApplication.h"
#include "CookieJar.h"
#include "Settings.h"

CookieJar::CookieJar(bool enableCookies, bool privateJar, QObject *parent) :
    QNetworkCookieJar(parent),
    m_enableCookies(enableCookies),
    m_privateJar(privateJar),
    m_store(nullptr),
    m_exemptParties()
{
    if (!m_privateJar)
        m_store = QWebEngineProfile::defaultProfile()->cookieStore();
    else
        m_store = sBrowserApplication->getPrivateBrowsingProfile()->cookieStore();

    connect(m_store, &QWebEngineCookieStore::cookieAdded, this, &CookieJar::onCookieAdded);
    connect(m_store, &QWebEngineCookieStore::cookieRemoved, [=](const QNetworkCookie &cookie){
        static_cast<void>(deleteCookie(cookie));
    });

    if (!m_privateJar)
        loadExemptThirdParties();
}

CookieJar::~CookieJar()
{
    disconnect(m_store, 0, 0, 0);

    if (!m_privateJar)
        saveExemptThirdParties();
}

bool CookieJar::hasCookiesFor(const QString &host) const
{
    if (host.isEmpty())
        return false;

    // If host string of format "xxx.yyy.zzz", must check for any cookies ending
    // in ".yyy.zzz"
    QString hostSearch = host;
    int numDots = host.count(QChar('.'));
    if (numDots > 1)
        hostSearch = host.right(host.size() - host.indexOf(QChar('.'), 0));

    auto cookies = allCookies();
    for (const QNetworkCookie &cookie : cookies)
        if (cookie.domain().endsWith(hostSearch))
            return true;
    return false;
}

void CookieJar::eraseAllCookies()
{
    QList<QNetworkCookie> noCookies;
    setAllCookies(noCookies);
    m_store->deleteAllCookies();

    emit cookiesRemoved();
}

void CookieJar::setCookiesEnabled(bool value)
{
    m_enableCookies = value;
    if (!m_enableCookies)
        eraseAllCookies();
}

void CookieJar::setThirdPartyCookiesEnabled(bool value)
{
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    if (value)
        m_store->setCookieFilter(nullptr);
    else
        m_store->setCookieFilter([=](const QWebEngineCookieStore::FilterRequest &request) {
            if (request.thirdParty && m_enableCookies)
            {
                URL originUrl(request.origin);
                const QString originDomain = originUrl.getSecondLevelDomain();
                for (auto &url : m_exemptParties)
                {
                    if (url.getSecondLevelDomain() == originDomain)
                        return true;
                }
                return false;
            }
            return m_enableCookies;
        });
#endif
}

const QSet<URL> &CookieJar::getExemptThirdPartyHosts() const
{
    return m_exemptParties;
}

void CookieJar::addThirdPartyExemption(const QUrl &hostUrl)
{
    m_exemptParties.insert(hostUrl);
}

void CookieJar::removeThirdPartyExemption(const QUrl &hostUrl)
{
    m_exemptParties.remove(hostUrl);
}

void CookieJar::loadExemptThirdParties()
{
    QFile exemptFile(sBrowserApplication->getSettings()->getPathValue(QLatin1String("ExemptThirdPartyCookieFile")));
    if (!exemptFile.exists() || !exemptFile.open(QIODevice::ReadOnly))
        return;

    m_exemptParties.clear();

    QString data(exemptFile.readAll());

    // Parse the simple newline-separated host list format
    QStringList exemptHostList = data.split(QChar('\n'));
    for (const QString &host : exemptHostList)
    {
        if (!host.isEmpty())
            m_exemptParties.insert(URL(host));
    }
}

void CookieJar::saveExemptThirdParties()
{
    if (m_exemptParties.empty())
        return;

    QFile exemptFile(sBrowserApplication->getSettings()->getPathValue(QLatin1String("ExemptThirdPartyCookieFile")));
    if (!exemptFile.open(QIODevice::WriteOnly))
        return;

    QTextStream out(&exemptFile);

    for (const auto &url : m_exemptParties)
    {
        out << url.getSecondLevelDomain() << '\n';
    }
    out.flush();
    exemptFile.close();
}

void CookieJar::onCookieAdded(const QNetworkCookie &cookie)
{
    if (m_enableCookies)
        static_cast<void>(insertCookie(cookie));
    else
        m_store->deleteCookie(cookie);
}

void CookieJar::removeExpired()
{
    if (m_privateJar)
        return;

    QList<QNetworkCookie> cookies = allCookies();
    if (cookies.empty())
        return;

    QDateTime now = QDateTime::currentDateTime();
    int numCookies = cookies.size();
    for (int i = numCookies - 1; i >= 0; --i)
    {
        const QNetworkCookie &cookie = cookies.at(i);
        QDateTime expireDate = cookie.expirationDate();
        if (cookie.isSessionCookie() || expireDate > now)
            continue;

        cookies.removeAt(i);
    }

    setAllCookies(cookies);
}
