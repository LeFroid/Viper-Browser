#include "DownloadManager.h"
#include "UserScriptModel.h"
#include "InternalDownloadItem.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>
#include <QUrl>
#include <QNetworkRequest>

UserScriptModel::UserScriptModel(DownloadManager *downloadManager, Settings *settings, QObject *parent) :
    QAbstractTableModel(parent),
    m_scripts(),
    m_scriptTemplate(),
    m_scriptDepDir(),
    m_enabled(false),
    m_downloadManager(downloadManager),
    m_settings(settings)
{
    m_enabled = settings->getValue(BrowserSetting::UserScriptsEnabled).toBool();
    if (m_enabled)
        load();
}

UserScriptModel::~UserScriptModel()
{
    if (m_enabled)
        save();
}

void UserScriptModel::addScript(const UserScript &script)
{
    int rowNum = rowCount();
    beginInsertRows(QModelIndex(), rowNum, rowNum);
    m_scripts.push_back(script);
    loadDependencies(rowNum);
    endInsertRows();
}

void UserScriptModel::addScript(UserScript &&script)
{
    int rowNum = rowCount();
    beginInsertRows(QModelIndex(), rowNum, rowNum);
    m_scripts.push_back(script);
    loadDependencies(rowNum);
    endInsertRows();
}

QString UserScriptModel::getScriptFileName(int indexRow) const
{
    if (indexRow < 0 || indexRow >= rowCount())
        return QString();

    return m_scripts.at(indexRow).m_fileName;
}

QString UserScriptModel::getScriptSource(int indexRow)
{
    if (indexRow < 0 || indexRow >= rowCount())
        return QString();

    QFile f(m_scripts.at(indexRow).m_fileName);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return QString();

    QByteArray data = f.readAll();
    f.close();
    return QString(data);
}

QVariant UserScriptModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return QString("On");
            case 1: return QString("Name");
            case 2: return QString("Description");
            case 3: return QString("Version");
            default: return QVariant();
        }
    }

    return QVariant();
}

int UserScriptModel::rowCount(const QModelIndex &/*parent*/) const
{
    return static_cast<int>(m_scripts.size());
}

int UserScriptModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 4;
}

QVariant UserScriptModel::data(const QModelIndex &index, int role) const
{
    if (index.row() >= rowCount())
        return QVariant();

    // Column is 0 for checkbox state
    if(index.column() == 0 && role == Qt::CheckStateRole)
    {
        return (m_scripts.at(index.row()).isEnabled() ? Qt::Checked : Qt::Unchecked);
    }

    if(role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    if (!index.isValid())
        return QVariant();

    const UserScript &script = m_scripts.at(index.row());
    switch (index.column())
    {
        case 1: return script.getName();
        case 2: return script.getDescription();
        case 3: return script.getVersion();
        default: return QVariant();
    }
}

Qt::ItemFlags UserScriptModel::flags(const QModelIndex& index) const
{
    if (index.column() == 0)
        return Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | QAbstractTableModel::flags(index);
    return QAbstractTableModel::flags(index);
}

bool UserScriptModel::setData(const QModelIndex &index, const QVariant &/*value*/, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole)
    {
        UserScript &script = m_scripts[index.row()];
        script.setEnabled(!script.isEnabled());
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

bool UserScriptModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent, row, row + count - 1);
    endInsertRows();
    return true;
}

bool UserScriptModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row + count > rowCount())
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    for (auto it = m_scripts.begin() + row; it != m_scripts.begin() + row + count;)
    {
        // Delete file and remove from script container
        QFile f(it->m_fileName);
        if (f.exists())
            f.remove();
        it = m_scripts.erase(it);
    }
    endRemoveRows();
    return true;
}

void UserScriptModel::reloadScript(int indexRow)
{
    if (indexRow < 0 || indexRow >= rowCount())
        return;

    UserScript &script = m_scripts.at(indexRow);
    if (script.load(script.m_fileName, m_scriptTemplate))
        loadDependencies(indexRow);
}

void UserScriptModel::load()
{
    // Load user script template
    QFile templateFile(":/GreaseMonkeyAPI.js");
    if (!templateFile.exists() || !templateFile.open(QIODevice::ReadOnly))
        return;

    m_scriptTemplate = QString(templateFile.readAll());
    templateFile.close();

    // Attempt to load statuses of user scripts, storing results in temporary hash map
    QFile scriptMgrFile(m_settings->getPathValue(BrowserSetting::UserScriptsConfig));
    QHash<QString, bool> scriptStatuses;
    if (scriptMgrFile.exists() && scriptMgrFile.open(QIODevice::ReadOnly))
    {
        // Attempt to parse status file
        QByteArray userScriptInfo = scriptMgrFile.readAll();
        scriptMgrFile.close();

        // For each file, determine if it is enabled or not
        QJsonDocument scriptInfoDoc(QJsonDocument::fromJson(userScriptInfo));
        QJsonObject scriptInfoObj = scriptInfoDoc.object();
        for (auto it = scriptInfoObj.begin(); it != scriptInfoObj.end(); ++it)
        {
            auto itVal = it.value();
            if (!itVal.isObject())
                continue;

            QString scriptFileName = it.key();
            bool status = itVal.toObject().value(QStringLiteral("enabled")).toBool(true);

            scriptStatuses[scriptFileName] = status;
        }
    }

    // Load user scripts
    QDir scriptDir(m_settings->getPathValue(BrowserSetting::UserScriptsDir));
    if (!scriptDir.exists())
        scriptDir.mkpath(scriptDir.absolutePath());

    m_userScriptDir = scriptDir.absolutePath();

    QDir scriptDepDir(QString("%1/Dependencies").arg(m_userScriptDir));
    if (!scriptDepDir.exists())
        scriptDepDir.mkpath(scriptDepDir.absolutePath());

    m_scriptDepDir = scriptDepDir.absolutePath();

    QDirIterator scriptDirItr(m_userScriptDir, QDir::Files);
    while (scriptDirItr.hasNext())
    {
        UserScript script;
        if (script.load(scriptDirItr.next(), m_scriptTemplate))
        {
            auto statusIt = scriptStatuses.find(script.m_fileName);
            if (statusIt != scriptStatuses.end())
                script.m_isEnabled = statusIt.value();
            m_scripts.push_back(script);
        }
    }

    int numScripts = static_cast<int>(m_scripts.size());
    for (int i = 0; i < numScripts; ++i)
        loadDependencies(i);
}

void UserScriptModel::loadDependencies(int scriptIdx)
{
    if (scriptIdx < 0 || scriptIdx >= static_cast<int>(m_scripts.size()))
        return;

    UserScript &script = m_scripts.at(scriptIdx);

    for (const QString &depFile : script.m_dependencies)
    {
        QString localFilePath;
        if (!depFile.contains("://"))
            continue;

        localFilePath = QString("%1/%2").arg(m_scriptDepDir, depFile.mid(depFile.lastIndexOf('/') + 1));
        QFile f(localFilePath);
        if (!f.exists() || !f.open(QIODevice::ReadOnly))
        {
            if (!m_downloadManager)
                return;

            QNetworkRequest request;
            request.setUrl(QUrl(depFile));
            InternalDownloadItem *item = m_downloadManager->downloadInternal(request, m_scriptDepDir, false);
            connect(item, &InternalDownloadItem::downloadFinished, [=](const QString &filePath){
                QFile tmp(filePath);
                QByteArray tmpData;
                if (tmp.open(QIODevice::ReadOnly))
                {
                    tmpData = tmp.readAll();
                    m_scripts[scriptIdx].m_dependencyData.append(tmpData);
                    tmp.close();
                }
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

void UserScriptModel::save()
{
    QFile dataFile(m_settings->getPathValue(BrowserSetting::UserScriptsConfig));
    if (!dataFile.open(QIODevice::WriteOnly))
        return;

    // Construct a JSON object containing the file names of each script, and an object for each stating whether or not the script is enabled
    QJsonObject scriptMgrObj;
    for (const UserScript &script : m_scripts)
    {
        QJsonObject scriptObj;
        scriptObj.insert(QStringLiteral("enabled"), QJsonValue(script.m_isEnabled));
        scriptMgrObj.insert(script.m_fileName, QJsonValue(scriptObj));
    }
    QJsonDocument scriptMgrDoc(scriptMgrObj);
    dataFile.write(scriptMgrDoc.toJson());
    dataFile.close();
}
