#include "SchemeRegistry.h"

#include <QtGlobal>
#include <QtWebEngineCoreVersion>

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 12, 0))
#include <QWebEngineUrlScheme>
#endif

const std::array<QString, 2> SchemeRegistry::SecureSchemes = { QStringLiteral("https"), QStringLiteral("viper") };

const std::array<QString, 4> SchemeRegistry::WhitelistedSchemes = { QStringLiteral("blocked"),
                                                                    QStringLiteral("file"),
                                                                    QStringLiteral("qrc"),
                                                                    QStringLiteral("viper") };

void SchemeRegistry::registerSchemes()
{
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    QWebEngineUrlScheme blockedScheme("blocked");
    blockedScheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);

    QWebEngineUrlScheme::Flags blockedFlags = QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::ContentSecurityPolicyIgnored;
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    blockedFlags |= QWebEngineUrlScheme::CorsEnabled;
#endif
    blockedScheme.setFlags(blockedFlags);
    QWebEngineUrlScheme::registerScheme(blockedScheme);

    QWebEngineUrlScheme viperScheme("viper");
    QWebEngineUrlScheme::Flags viperFlags = QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalAccessAllowed | QWebEngineUrlScheme::ContentSecurityPolicyIgnored;
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    viperFlags |= QWebEngineUrlScheme::CorsEnabled;
#endif
    viperScheme.setFlags(viperFlags);
    QWebEngineUrlScheme::registerScheme(viperScheme);
#endif
}

bool SchemeRegistry::isSecure(const QString &scheme)
{
    return std::find(SecureSchemes.begin(), SecureSchemes.end(), scheme) != SecureSchemes.end();
}

bool SchemeRegistry::isSchemeWhitelisted(const QString &scheme)
{
    return std::find(WhitelistedSchemes.begin(), WhitelistedSchemes.end(), scheme) != WhitelistedSchemes.end();
}
