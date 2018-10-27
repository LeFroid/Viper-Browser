#include "SchemeRegistry.h"

#include <QtGlobal>
#include <QtWebEngineCoreVersion>

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 12, 0))
#include <QWebEngineUrlScheme>
#endif

void SchemeRegistry::registerSchemes()
{
#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 12, 0))
    QWebEngineUrlScheme scheme("blocked");
    scheme.setSyntax(QWebEngineUrlScheme::Syntax::Path);
    scheme.setFlags(QWebEngineUrlScheme::SecureScheme | QWebEngineUrlScheme::ContentSecurityPolicyIgnored);
    QWebEngineUrlScheme::registerScheme(scheme);
#endif
}
