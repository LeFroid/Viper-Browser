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

CookieJar::CookieJar(Settings *settings, QWebEngineProfile *defaultProfile, bool privateJar, QObject *parent) :
    QNetworkCookieJar(parent),
    m_enableCookies(false),
    m_privateJar(privateJar),
    m_store(nullptr),
    m_exemptParties(),
    m_exemptThirdPartyCookieFileName(),
    m_mutex()
{
    setObjectName(QLatin1String("CookieJar"));

    if (settings)
    {
        m_enableCookies = settings->getValue(BrowserSetting::EnableCookies).toBool();
        m_exemptThirdPartyCookieFileName = settings->getPathValue(BrowserSetting::ExemptThirdPartyCookieFile);

        // Subscribe to settings changes
        connect(settings, &Settings::settingChanged, this, &CookieJar::onSettingChanged);
    }

    if (!m_privateJar)
        m_store = defaultProfile->cookieStore();
    else
        m_store = sBrowserApplication->getPrivateBrowsingProfile()->cookieStore();

    connect(m_store, &QWebEngineCookieStore::cookieAdded, this, &CookieJar::onCookieAdded);
    connect(m_store, &QWebEngineCookieStore::cookieRemoved, this, &CookieJar::onCookieRemoved);

    if (!m_privateJar)
        loadExemptThirdParties();

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    connect(sBrowserApplication, &BrowserApplication::aboutToQuit, this, [this]() {
        if (m_store)
            m_store->setCookieFilter(nullptr);
    });

    if (settings)
        setThirdPartyCookiesEnabled(settings->getValue(BrowserSetting::EnableThirdPartyCookies).toBool());
#endif
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
    {
        m_store->setCookieFilter(nullptr);
        return;
    }

    m_store->setCookieFilter([this](const QWebEngineCookieStore::FilterRequest &request) -> bool {
        if (request.thirdParty && m_enableCookies)
        {
            const QString originHost = request.origin.host();
            for (const auto &url : qAsConst(m_exemptParties))
            {
                const QString urlHost = url.host();
                if (urlHost == originHost)
                    return true;

                const int domainIndex = originHost.indexOf(urlHost);
                if (domainIndex > 0 && originHost.at(domainIndex - 1) == QLatin1Char('.'))
                    return true;
            }
            return false;
        }
        return m_enableCookies;
    });
#else
    Q_UNUSED(value);
#endif
}

const QSet<URL> &CookieJar::getExemptThirdPartyHosts() const
{
    return m_exemptParties;
}

void CookieJar::addThirdPartyExemption(const QUrl &hostUrl)
{
    URL url(hostUrl);
    m_exemptParties.insert(url);
}

void CookieJar::removeThirdPartyExemption(const QUrl &hostUrl)
{
    URL url(hostUrl);
    m_exemptParties.remove(hostUrl);
}

void CookieJar::loadExemptThirdParties()
{
    QFile exemptFile(m_exemptThirdPartyCookieFileName);
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

    QFile exemptFile(m_exemptThirdPartyCookieFileName);
    if (!exemptFile.open(QIODevice::WriteOnly))
        return;

    QTextStream out(&exemptFile);

    for (const auto &url : qAsConst(m_exemptParties))
    {
        QString scheme = url.scheme();
        if (scheme.isEmpty())
            scheme = QString("https");

        QString host = url.host();
        if (host.isEmpty())
            host = url.toString(URL::EncodeUnicode);

        QString output = QString("%1://%2").arg(scheme, host);
        out << output << '\n';
    }
    out.flush();
    exemptFile.close();
}

void CookieJar::onCookieAdded(const QNetworkCookie &cookie)
{
    try {
        std::lock_guard<std::mutex> _(m_mutex);

        if (m_enableCookies)
            static_cast<void>(insertCookie(cookie));
        else
            m_store->deleteCookie(cookie);
    } catch (const std::exception &ex) {
        qDebug() << "CookieJar::onCookieAdded - caught exception" << ex.what();
    }
}

void CookieJar::onCookieRemoved(const QNetworkCookie &cookie)
{
    std::lock_guard<std::mutex> _(m_mutex);
    static_cast<void>(deleteCookie(cookie));
}

void CookieJar::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::EnableCookies)
    {
        const bool enabled = value.toBool();
        if (enabled != m_enableCookies)
            setCookiesEnabled(enabled);
    }
    else if (setting == BrowserSetting::ExemptThirdPartyCookieFile)
    {
        if (Settings *settings = qobject_cast<Settings*>(sender()))
            m_exemptThirdPartyCookieFileName = settings->getPathValue(BrowserSetting::ExemptThirdPartyCookieFile);
    }
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    else if (setting == BrowserSetting::EnableThirdPartyCookies)
    {
        setThirdPartyCookiesEnabled(value.toBool());
    }
#endif
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
