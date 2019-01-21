#include "BrowserScripts.h"

#include <QFile>

BrowserScripts::BrowserScripts() :
    m_globalScripts(),
    m_publicOnlyScripts()
{
    initPrintScript();
    initWebChannelScript();
    initFaviconScript();
    initNewTabScript();
    initAutoFillObserverScript();
}

const std::vector<QWebEngineScript> &BrowserScripts::getGlobalScripts() const
{
    return m_globalScripts;
}

const std::vector<QWebEngineScript> &BrowserScripts::getPublicOnlyScripts() const
{
    return m_publicOnlyScripts;
}

void BrowserScripts::initPrintScript()
{
    QWebEngineScript printScript;
    printScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
    printScript.setName(QLatin1String("viper-window-script"));
    printScript.setRunsOnSubFrames(true);
    printScript.setWorldId(QWebEngineScript::MainWorld);
    printScript.setSourceCode(QLatin1String("(function() { window.print = function() { "
                                            "window.location = 'viper:print'; }; })()"));

    m_globalScripts.push_back(printScript);
}

void BrowserScripts::initWebChannelScript()
{
    QString webChannelJS;
    QFile webChannelFile(QLatin1String(":/qtwebchannel/qwebchannel.js"));
    if (webChannelFile.open(QIODevice::ReadOnly))
        webChannelJS = webChannelFile.readAll();
    webChannelFile.close();

    QString webChannelSetup;
    QFile webChannelSetupFile(QLatin1String(":/WebChannelSetup.js"));
    if (webChannelSetupFile.open(QIODevice::ReadOnly))
    {
        webChannelSetup = webChannelSetupFile.readAll();
        webChannelSetup = webChannelSetup.arg(webChannelJS);
    }
    webChannelSetupFile.close();

    QWebEngineScript webChannelScript;
    webChannelScript.setInjectionPoint(QWebEngineScript::DocumentCreation);
    webChannelScript.setName(QLatin1String("viper-web-channel"));
    webChannelScript.setRunsOnSubFrames(true);
    webChannelScript.setWorldId(QWebEngineScript::ApplicationWorld);
    webChannelScript.setSourceCode(webChannelSetup);

    m_globalScripts.push_back(webChannelScript);
}

void BrowserScripts::initFaviconScript()
{
    QString faviconScript;
    QFile faviconScriptFile(QLatin1String(":/GetFavicon.js"));
    if (faviconScriptFile.open(QIODevice::ReadOnly))
        faviconScript = faviconScriptFile.readAll();
    faviconScriptFile.close();

    QWebEngineScript faviconBridgeScript;
    faviconBridgeScript.setInjectionPoint(QWebEngineScript::DocumentReady);
    faviconBridgeScript.setName(QLatin1String("viper-favicon-bridge"));
    faviconBridgeScript.setRunsOnSubFrames(false);
    faviconBridgeScript.setWorldId(QWebEngineScript::ApplicationWorld);
    faviconBridgeScript.setSourceCode(faviconScript);

    m_globalScripts.push_back(faviconBridgeScript);
}

void BrowserScripts::initNewTabScript()
{
    QString newTabJS;
    QFile newTabJSFile(QLatin1String(":/NewTabPage.js"));
    if (newTabJSFile.open(QIODevice::ReadOnly))
        newTabJS = newTabJSFile.readAll();
    newTabJSFile.close();

    QWebEngineScript newTabScript;
    newTabScript.setInjectionPoint(QWebEngineScript::DocumentReady);
    newTabScript.setName(QLatin1String("viper-new-tab-script"));
    newTabScript.setRunsOnSubFrames(false);
    newTabScript.setWorldId(QWebEngineScript::ApplicationWorld);
    newTabScript.setSourceCode(newTabJS);

    m_publicOnlyScripts.push_back(newTabScript);
}

void BrowserScripts::initAutoFillObserverScript()
{
    QString autoFillJS;
    QFile autoFillObserverFile(QLatin1String(":/AutoFillObserver.js"));
    if (autoFillObserverFile.open(QIODevice::ReadOnly))
        autoFillJS = autoFillObserverFile.readAll();
    autoFillObserverFile.close();

    QWebEngineScript autoFillObserverScript;
    autoFillObserverScript.setInjectionPoint(QWebEngineScript::DocumentReady);
    autoFillObserverScript.setName(QLatin1String("viper-autofill-observer"));
    autoFillObserverScript.setRunsOnSubFrames(true);
    autoFillObserverScript.setWorldId(QWebEngineScript::ApplicationWorld);
    autoFillObserverScript.setSourceCode(autoFillJS);

    m_publicOnlyScripts.push_back(autoFillObserverScript);
}
