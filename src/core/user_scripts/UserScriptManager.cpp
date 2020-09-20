#include "DownloadManager.h"
#include "UserScriptManager.h"
#include "UserScriptModel.h"
#include "InternalDownloadItem.h"
#include "WebEngineScriptAdapter.h"

#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QNetworkRequest>
#include <QUrl>

UserScriptManager::UserScriptManager(DownloadManager *downloadManager, Settings *settings) :
    QObject(nullptr),
    m_downloadManager(downloadManager),
    m_model(new UserScriptModel(downloadManager, settings, this))
{
    setObjectName(QLatin1String("UserScriptManager"));
    connect(settings, &Settings::settingChanged, this, &UserScriptManager::onSettingChanged);
}

UserScriptManager::~UserScriptManager()
{
}

void UserScriptManager::setEnabled(bool value)
{
    m_model->m_enabled = value;
}

UserScriptModel *UserScriptManager::getModel()
{
    return m_model;
}

QString UserScriptManager::getScriptsFor(const QUrl &url, ScriptInjectionTime injectionTime, bool isMainFrame)
{
    if (!m_model->m_enabled)
        return QString();

    QByteArray resultBuffer;
    QString urlStr = url.toString(QUrl::FullyEncoded);

    for (const UserScript &script : m_model->m_scripts)
    {
        if (!script.m_isEnabled
                || (injectionTime != script.m_injectionTime)
                || (script.m_noSubFrames && !isMainFrame))
            continue;

        bool isExclude = false, isInclude = false;
        for (const QRegularExpression &expr : script.m_excludes)
        {
            if (expr.match(urlStr).hasMatch())
            {
                isExclude = true;
                break;
            }
        }

        if (isExclude)
            continue;

        for (const QRegularExpression &expr : script.m_includes)
        {
            if (expr.match(urlStr).hasMatch())
            {
                isInclude = true;
                break;
            }
        }

        if (isInclude)
        {
            resultBuffer.append(script.m_dependencyData);
            resultBuffer.append('\n');
            resultBuffer.append(script.m_scriptData.toUtf8());
        }
    }

    return QString(resultBuffer);
}

std::vector<QWebEngineScript> UserScriptManager::getAllScriptsFor(const QUrl &url)
{
    std::vector<QWebEngineScript> result;
    if (!m_model->m_enabled)
        return result;

    QString urlStr = url.toString(QUrl::FullyEncoded);
    for (const UserScript &script : m_model->m_scripts)
    {
        if (!script.m_isEnabled)
            continue;

        bool isExclude = false, isInclude = false;
        for (const QRegularExpression &expr : script.m_excludes)
        {
            if (expr.match(urlStr).hasMatch())
            {
                isExclude = true;
                break;
            }
        }

        if (isExclude)
            continue;

        for (const QRegularExpression &expr : script.m_includes)
        {
            if (expr.match(urlStr).hasMatch())
            {
                isInclude = true;
                break;
            }
        }

        if (isInclude)
        {
            WebEngineScriptAdapter scriptAdapter(script);
            result.push_back(scriptAdapter.getScript());
        }
    }
    return result;
}

void UserScriptManager::installScript(const QUrl &url)
{
    if (!url.isValid() || !m_downloadManager)
        return;

    QNetworkRequest request;
    request.setUrl(url);

    InternalDownloadItem *item = m_downloadManager->downloadInternal(request, m_model->m_userScriptDir, false);
    connect(item, &InternalDownloadItem::downloadFinished, [this](const QString &filePath){
        UserScript script;
        if (script.load(filePath, m_model->m_scriptTemplate))
            m_model->addScript(std::move(script));
    });
}

void UserScriptManager::createScript(const QString &name, const QString &nameSpace, const QString &description, const QString &version)
{
    // Require a name and namespace
    if (name.isEmpty() || nameSpace.isEmpty())
        return;

    QString fileName = QString("%1%2%3.%4.user.js").arg(m_model->m_userScriptDir, QDir::separator(), nameSpace, name);
    QFile f(fileName);
    if (f.exists())
    {
        fileName = QString("%1%2%3.%4.user.js").arg(m_model->m_userScriptDir, QDir::separator(), name, nameSpace);
        f.setFileName(fileName);

        // Exit if the second attempt to find an unused name fails
        if (f.exists())
            return;
    }

    if (!f.open(QIODevice::WriteOnly))
        return;

    // Create metadata block and write to file
    QByteArray scriptData;
    scriptData.append("// ==UserScript==\n");
    scriptData.append("// @name    ").append(name.toUtf8()).append('\n');
    scriptData.append("// @namespace    ").append(nameSpace.toUtf8()).append('\n');
    scriptData.append("// @description    ").append(description.toUtf8()).append('\n');
    scriptData.append("// @version    ").append(version.toUtf8()).append('\n');
    scriptData.append("// ==/UserScript==\n\n");
    scriptData.append("// Don't forget to add @include, @exclude and/or @match rules to the script header!");
    f.write(scriptData);
    f.close();

    UserScript script;
    if (script.load(fileName, m_model->m_scriptTemplate))
    {
        m_model->addScript(std::move(script));
        emit scriptCreated(m_model->rowCount() - 1);
    }
}

void UserScriptManager::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::UserScriptsEnabled)
        setEnabled(value.toBool());
}
