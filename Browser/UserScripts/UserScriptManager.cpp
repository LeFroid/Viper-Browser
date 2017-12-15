#include "UserScriptManager.h"
#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "DownloadItem.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QUrl>
#include <QNetworkRequest>

UserScriptManager::UserScriptManager(std::shared_ptr<Settings> settings, QObject *parent) :
    QObject(parent),
    m_settings(settings),
    m_scripts(),
    m_enabled(false),
    m_scriptTemplate(),
    m_scriptDepDir()
{
    m_enabled = settings->getValue("UserScriptsEnabled").toBool();
    if (m_enabled)
        load();
}

QString UserScriptManager::getScriptsFor(const QUrl &url, ScriptInjectionTime injectionTime, bool isMainFrame)
{
    QByteArray resultBuffer;
    QString urlStr = url.toString(QUrl::FullyEncoded);

    for (const UserScript &script : m_scripts)
    {
        if (injectionTime != script.m_injectionTime || (script.m_noSubFrames && !isMainFrame))
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

void UserScriptManager::load()
{
    // Load user script template
    QFile templateFile(":/GreaseMonkeyAPI.js");
    if (!templateFile.exists() || !templateFile.open(QIODevice::ReadOnly))
        return;

    m_scriptTemplate = QString(templateFile.readAll());
    templateFile.close();

    // Load user scripts
    QDir scriptDir(m_settings->getPathValue("UserScriptsDir"));
    if (!scriptDir.exists())
        scriptDir.mkpath(scriptDir.absolutePath());

    QDir scriptDepDir(scriptDir);
    if (!scriptDepDir.cd("Dependencies"))
    {
        scriptDir.mkpath(QString("%1%2Dependencies").arg(scriptDir.absolutePath()).arg(QDir::separator()));
        scriptDepDir.cd("Dependencies");
        m_scriptDepDir = scriptDir.absolutePath();
        m_scriptDepDir.append(QDir::separator());
        m_scriptDepDir.append(QStringLiteral("Dependencies"));
    }
    else
        m_scriptDepDir = scriptDepDir.absolutePath();

    QDirIterator scriptDirItr(scriptDir.absolutePath(), QDir::Files);
    while (scriptDirItr.hasNext())
    {
        UserScript script;
        if (script.load(scriptDirItr.next(), m_scriptTemplate))
        {
            m_scripts.push_back(script);
        }
    }

    int numScripts = static_cast<int>(m_scripts.size());
    for (int i = 0; i < numScripts; ++i)
        loadDependencies(i);

    /* config file: load a format like the following
     * {
     *     "scriptname1.js": "http://path.com/where_script_was_downloaded/scriptname1.js",
     *     "scriptname2.js": "http://...",
     *     "scriptname3.js": "" (empty string if user-created)
     * }
     */
}

void UserScriptManager::loadDependencies(int scriptIdx)
{
    if (scriptIdx < 0 || scriptIdx >= static_cast<int>(m_scripts.size()))
        return;

    UserScript &script = m_scripts.at(scriptIdx);
    for (const QString &depFile : script.m_dependencies)
    {
        QString localFilePath;
        if (!depFile.contains("://"))
            continue;

        localFilePath = QString("%1%2%3").arg(m_scriptDepDir).arg(QDir::separator()).arg(depFile.mid(depFile.lastIndexOf('/') + 1));
        QFile f(localFilePath);
        if (!f.exists() || !f.open(QIODevice::ReadOnly))
        {
            DownloadManager *downloadMgr = sBrowserApplication->getDownloadManager();
            const QString oldDownloadDir = downloadMgr->getDownloadDir();
            downloadMgr->setDownloadDir(m_scriptDepDir);

            QNetworkRequest request;
            request.setUrl(QUrl(depFile));
            DownloadItem *item = downloadMgr->downloadInternal(request, false, false);
            connect(item, &DownloadItem::downloadFinished, [=](const QString &filePath){
                QFile tmp(filePath);
                QByteArray tmpData;
                if (tmp.open(QIODevice::ReadOnly))
                {
                    tmpData = tmp.readAll();
                    m_scripts[scriptIdx].m_dependencyData.append(tmpData);
                    tmp.close();
                }
                downloadMgr->setDownloadDir(oldDownloadDir);
                item->deleteLater();
            });
        }
        else
        {
            script.m_dependencyData.append(f.readAll());
            f.close();
        }
    }
}

void UserScriptManager::save()
{

}
