#include "DownloadManager.h"
#include "ui_downloadmanager.h"
#include "DownloadItem.h"
#include "NetworkAccessManager.h"

#include <QDir>
#include <QNetworkReply>

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

void DownloadManager::download(const QNetworkRequest &request, bool askForFileName)
{
    if (!m_accessMgr || request.url().isEmpty())
        return;

    handleUnsupportedContent(m_accessMgr->get(request), askForFileName);
}

DownloadItem *DownloadManager::downloadInternal(const QNetworkRequest &request, const QString &downloadDir, bool askForFileName, bool writeOverExisting)
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

    DownloadItem *item = new DownloadItem(reply, downloadDir, askForFileName, writeOverExisting, this);
    return item;
}

void DownloadManager::handleUnsupportedContent(QNetworkReply *reply, bool askForFileName)
{
    // Make sure content is not empty
    QVariant lenHeader = reply->header(QNetworkRequest::ContentLengthHeader);
    bool castOk;
    int contentLen = lenHeader.toInt(&castOk);
    if (castOk && !contentLen)
        return;

    // Add to table
    DownloadItem *item = new DownloadItem(reply, m_downloadDir, askForFileName, false, this);
    int downloadRow = m_downloads.size();
    m_downloads.append(item);
    ui->tableWidget->insertRow(downloadRow);
    ui->tableWidget->setCellWidget(downloadRow, 0, item);
    ui->tableWidget->setRowHeight(downloadRow, item->sizeHint().height());

    // Show download manager if hidden
    if (!isVisible())
        show();
}
