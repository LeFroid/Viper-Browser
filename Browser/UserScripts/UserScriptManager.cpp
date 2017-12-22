#include "UserScriptManager.h"
#include "UserScriptModel.h"
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
    m_model(nullptr)
{
    m_model = new UserScriptModel(settings, this);
}

UserScriptManager::~UserScriptManager()
{
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

void UserScriptManager::installScript(const QUrl &url)
{
    if (!url.isValid())
        return;

    QNetworkRequest request;
    request.setUrl(url);

    DownloadManager *downloadMgr = sBrowserApplication->getDownloadManager();
    DownloadItem *item = downloadMgr->downloadInternal(request, m_model->m_userScriptDir, false);
    connect(item, &DownloadItem::downloadFinished, [=](const QString &filePath){
        UserScript script;
        if (script.load(filePath, m_model->m_scriptTemplate))
            m_model->addScript(std::move(script));
    });
}

