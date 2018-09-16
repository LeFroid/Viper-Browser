#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "ui_downloadmanager.h"
#include "DownloadItem.h"
#include "InternalDownloadItem.h"
#include "NetworkAccessManager.h"
#include "Settings.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QNetworkReply>
#include <QWebEngineDownloadItem>

DownloadManager::DownloadManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadManager),
    m_downloadDir(),
    m_accessMgr(nullptr),
    m_downloads()
{
    ui->setupUi(this);

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

void DownloadManager::onDownloadRequest(QWebEngineDownloadItem *item)
{
    if (!item || item->state() != QWebEngineDownloadItem::DownloadRequested)
        return;

    QString fileName;

    // Check settings to determine if download path should be set by the user or handled by the web engine
    const bool askWhereToSave = sBrowserApplication->getSettings()->getValue(BrowserSetting::AskWhereToSaveDownloads).toBool();

    // Get file path for download
    if (askWhereToSave)
    {
        fileName = QFileInfo(item->path()).fileName();
        QString downloadPath = QString(m_downloadDir + QDir::separator() + fileName);
        fileName = QFileDialog::getSaveFileName(sBrowserApplication->activeWindow(), tr("Save as..."),  downloadPath);
        if (fileName.isEmpty())
            return;
    }
    else
        fileName = item->path();

    setDownloadDir(QFileInfo(fileName).absoluteDir().absolutePath());

    item->setPath(fileName);
    item->accept();

    DownloadItem *dlItem = new DownloadItem(item, this);

    int downloadRow = m_downloads.size();
    m_downloads.append(dlItem);
    connect(dlItem, &DownloadItem::removeFromList, [=](){
        int row = m_downloads.indexOf(dlItem);
        if (row >= 0)
        {
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
