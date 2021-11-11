#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "ui_DownloadManager.h"
#include "DownloadItem.h"
#include "InternalDownloadItem.h"
#include "NetworkAccessManager.h"
#include "Settings.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QNetworkReply>
#include <QWebEngineDownloadRequest>
#include <QWebEngineProfile>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

DownloadManager::DownloadManager(Settings *settings, const std::vector<QWebEngineProfile*> &webProfiles) :
    QWidget(nullptr),
    ui(new Ui::DownloadManager),
    m_downloadDir(),
    m_accessMgr(nullptr),
    m_askWhereToSaveDownloads(false),
    m_downloads()
{
    ui->setupUi(this);
    setObjectName(QLatin1String("DownloadManager"));
    setAttribute(Qt::WA_QuitOnClose, false);

    if (settings)
    {
        m_downloadDir = settings->getValue(BrowserSetting::DownloadDir).toString();
        m_askWhereToSaveDownloads = settings->getValue(BrowserSetting::AskWhereToSaveDownloads).toBool();
    }

    for (QWebEngineProfile *profile : webProfiles)
    {
        if (profile != nullptr)
        {
            connect(profile, &QWebEngineProfile::downloadRequested, this, &DownloadManager::onDownloadRequest);
        }
    }

    connect(ui->pushButtonClear, &QPushButton::clicked, this, &DownloadManager::clearDownloads);
}

DownloadManager::~DownloadManager()
{
    delete ui;
}

void DownloadManager::setDownloadDir(const QString &path)
{
    m_downloadDir = path;
}

const QString &DownloadManager::getDownloadDir() const
{
    return m_downloadDir;
}

void DownloadManager::setNetworkAccessManager(NetworkAccessManager *manager)
{
    m_accessMgr = manager;
}

void DownloadManager::clearDownloads()
{
    if (m_downloads.empty())
        return;

    for (int i = m_downloads.size() - 1; i >= 0; --i)
    {
        DownloadItem *item = m_downloads.at(i);
        ui->tableWidget->removeRow(i);
        m_downloads.removeAt(i);
        delete item;
    }
}

void DownloadManager::onDownloadRequest(QWebEngineDownloadRequest *item)
{
    if (!item || item->state() != QWebEngineDownloadRequest::DownloadRequested)
        return;

    QString fileName;

    // Get file name / directory
    if (m_askWhereToSaveDownloads)
    {
        fileName = QDir(m_downloadDir).filePath(item->suggestedFileName());
        fileName = QFileDialog::getSaveFileName(sBrowserApplication->activeWindow(), tr("Save as..."),  fileName);
    }
    else
    {
        fileName = QDir(item->downloadDirectory()).filePath(item->suggestedFileName());
    }

    if (fileName.isEmpty())
        return;

    QFileInfo fileInfo(fileName);
    m_downloadDir = fileInfo.absoluteDir().absolutePath();
    item->setDownloadDirectory(m_downloadDir);
    item->setDownloadFileName(fileInfo.fileName());
    item->accept();

    DownloadItem *dlItem = new DownloadItem(item, this);

    int downloadRow = m_downloads.size();
    m_downloads.append(dlItem);
    connect(dlItem, &DownloadItem::removeFromList, this, [this, dlItem](){
        int row = m_downloads.indexOf(dlItem);
        if (row >= 0)
        {
            if (!dlItem->isFinished())
                dlItem->cancel();

            ui->tableWidget->removeRow(row);
            m_downloads.removeAt(row);
            delete dlItem;
        }
    });

    ui->tableWidget->insertRow(downloadRow);
    ui->tableWidget->setCellWidget(downloadRow, 0, dlItem);
    ui->tableWidget->setRowHeight(downloadRow, dlItem->sizeHint().height());

    // Show download manager if hidden
    if (!isVisible())
        show();
}

InternalDownloadItem *DownloadManager::downloadInternal(const QNetworkRequest &request, const QString &downloadDir, bool askForFileName, bool writeOverExisting)
{
    if (!m_accessMgr || request.url().isEmpty())
        return nullptr;

    // Make sure content is not empty
    QNetworkReply *reply = m_accessMgr->get(request);
    QVariant lenHeader = reply->header(QNetworkRequest::ContentLengthHeader);
    bool castOk;
    int contentLen = lenHeader.toInt(&castOk);
    if (castOk && !contentLen)
        return nullptr;

    InternalDownloadItem *item = new InternalDownloadItem(reply, downloadDir, askForFileName, writeOverExisting, this);
    return item;
}

void DownloadManager::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::DownloadDir)
    {
        m_downloadDir = value.toString();
    }
    else if (setting == BrowserSetting::AskWhereToSaveDownloads)
    {
        m_askWhereToSaveDownloads = value.toBool();
    }
}
