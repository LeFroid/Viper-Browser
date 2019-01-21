#include "UserScriptManager.h"
#include "UserScriptModel.h"
#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "InternalDownloadItem.h"
#include "WebEngineScriptAdapter.h"

#include <QDir>
#include <QFile>

#include <QRegularExpression>
#include <QUrl>
#include <QNetworkRequest>

UserScriptManager::UserScriptManager(Settings *settings, QObject *parent) :
    QObject(parent),
    m_model(nullptr)
{
    m_model = new UserScriptModel(settings, this);
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
            resultBuffer.append(QChar('\n'));
            resultBuffer.append(script.m_scriptData);
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
    if (!url.isValid())
        return;

    QNetworkRequest request;
    request.setUrl(url);

    DownloadManager *downloadMgr = sBrowserApplication->getDownloadManager();
    InternalDownloadItem *item = downloadMgr->downloadInternal(request, m_model->m_userScriptDir, false);
    connect(item, &InternalDownloadItem::downloadFinished, [=](const QString &filePath){
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

    QString fileName = QString("%1%2%3.%4.user.js").arg(m_model->m_userScriptDir).arg(QDir::separator()).arg(nameSpace).arg(name);
    QFile f(fileName);
    if (f.exists())
    {
        fileName = QString("%1%2%3.%4.user.js").arg(m_model->m_userScriptDir).arg(QDir::separator()).arg(name).arg(nameSpace);
        f.setFileName(fileName);

        // Exit if the second attempt to find an unused name fails
        if (f.exists())
            return;
    }

    if (!f.open(QIODevice::WriteOnly))
        return;

    // Create metadata block and write to file
    QByteArray scriptData;
    scriptData.append(QString("// ==UserScript==\n"));
    scriptData.append(QString("// @name    %1\n").arg(name));
    scriptData.append(QString("// @namespace    %1\n").arg(nameSpace));
    scriptData.append(QString("// @description    %1\n").arg(description));
    scriptData.append(QString("// @version    %1\n").arg(version));
    scriptData.append(QString("// ==/UserScript==\n\n"));
    scriptData.append(QString("// Don't forget to add @include, @exclude and/or @match rules to the script header!"));
    f.write(scriptData);
    f.close();

    UserScript script;
    if (script.load(fileName, m_model->m_scriptTemplate))
    {
        m_model->addScript(std::move(script));
        emit scriptCreated(m_model->rowCount() - 1);
    }
}

