#include "WebEngineScriptAdapter.h"

#include <QByteArray>
#include <QString>

WebEngineScriptAdapter::WebEngineScriptAdapter(const UserScript &script) :
    m_script()
{
    m_script.setName(script.getName());
    m_script.setRunsOnSubFrames(script.isEnabledOnSubFrames());
    m_script.setWorldId(QWebEngineScript::ApplicationWorld);

    auto injectionTime = script.getInjectionTime();
    QWebEngineScript::InjectionPoint webEngineInjectionPoint = QWebEngineScript::DocumentCreation;
    switch (injectionTime)
    {
        case ScriptInjectionTime::DocumentStart:
            webEngineInjectionPoint = QWebEngineScript::DocumentCreation;
            break;
        case ScriptInjectionTime::DocumentEnd:
            webEngineInjectionPoint = QWebEngineScript::DocumentReady;
            break;
        case ScriptInjectionTime::DocumentIdle:
            webEngineInjectionPoint = QWebEngineScript::Deferred;
            break;
    }
    m_script.setInjectionPoint(webEngineInjectionPoint);

    QByteArray codeBuffer;
    codeBuffer.append(script.getDependencyData());
    codeBuffer.append('\n').append(script.getScriptData().toUtf8());

    m_script.setSourceCode(QString::fromUtf8(codeBuffer));
}

const QWebEngineScript &WebEngineScriptAdapter::getScript()
{
    return m_script;
}
