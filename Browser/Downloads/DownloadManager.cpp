#include "DownloadManager.h"
#include "ui_downloadmanager.h"
#include "DownloadItem.h"
#include "InternalDownloadItem.h"
#include "NetworkAccessManager.h"

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

void DownloadManager::onDownloadRequest(QWebEngineDownloadItem *item)
{
    if (!item || item->state() != QWebEngineDownloadItem::DownloadRequested)
        return;

    // Get file path for download
    QString fileName = QFileInfo(item->path()).fileName();
    QString downloadPath = QString(m_downloadDir + QDir::separator() + fileName);
    fileName = QFileDialog::getSaveFileName(this, tr("Save as..."),  downloadPath);
    if (fileName.isEmpty())
        return;

    setDownloadDir(QFileInfo(fileName).absoluteDir().absolutePath());

    item->setPath(fileName);
    item->accept();

    DownloadItem *dlItem = new DownloadItem(item, this);
    int downloadRow = m_downloads.size();
    m_downloads.append(dlItem);
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
