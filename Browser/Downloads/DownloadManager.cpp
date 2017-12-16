#include "DownloadManager.h"
#include "ui_downloadmanager.h"
#include "DownloadItem.h"
#include "DownloadListModel.h"
#include "NetworkAccessManager.h"

#include <QDir>
#include <QNetworkReply>

DownloadManager::DownloadManager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DownloadManager),
    m_downloadDir(),
    m_accessMgr(nullptr),
    m_model(new DownloadListModel(this)),
    m_downloads()
{
    ui->setupUi(this);
    ui->listView->setModel(m_model);
    m_model->setParent(ui->listView);
}

DownloadManager::~DownloadManager()
{
    delete ui;
}

void DownloadManager::setDownloadDir(const QString &path)
{
    m_downloadDir = path + QDir::separator();
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

DownloadItem *DownloadManager::downloadInternal(const QNetworkRequest &request, const QString &downloadDir, bool askForFileName)
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

    // Add to model
    DownloadItem *item = new DownloadItem(reply, downloadDir, askForFileName, this);
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

    // Add to model
    DownloadItem *item = new DownloadItem(reply, m_downloadDir, askForFileName, this);
    int downloadRow = m_downloads.size();
    m_model->beginInsertRows(QModelIndex(), downloadRow, downloadRow);
    m_downloads.append(item);
    m_model->endInsertRows();

    ui->listView->setIndexWidget(m_model->index(downloadRow, 0), item);

    // Show download manager if hidden
    if (!isVisible())
        show();
}
