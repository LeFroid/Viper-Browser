#include "SchemeRegistry.h"

#include <QtGlobal>
#include <QtWebEngineCoreVersion>

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 12, 0))
#include <QWebEngineUrlScheme>
#endif

const std::array<QString, 2> SchemeRegistry::SecureSchemes = { QStringLiteral("https"), QStringLiteral("viper") };

void SchemeRegistry::registerSchemes()
{
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    QWebEngineUrlScheme blockedScheme("blocked");
    blockedScheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    blockedScheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::ContentSecurityPolicyIgnored);
    QWebEngineUrlScheme::registerScheme(blockedScheme);

    QWebEngineUrlScheme viperScheme("viper");
    viperScheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::LocalAccessAllowed | QWebEngineUrlScheme::ContentSecurityPolicyIgnored);
    QWebEngineUrlScheme::registerScheme(viperScheme);
#endif
}

bool SchemeRegistry::isSecure(const QString &scheme)
{
    return std::find(SecureSchemes.begin(), SecureSchemes.end(), scheme) != SecureSchemes.end();
}
