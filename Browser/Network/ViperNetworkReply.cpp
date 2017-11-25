#include "ViperNetworkReply.h"

#include <algorithm>
#include <cstring>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QTimer>

ViperNetworkReply::ViperNetworkReply(const QNetworkRequest &request, QObject *parent) :
    QNetworkReply(parent),
    m_data(),
    m_offset(0)
{
    setOperation(QNetworkAccessManager::GetOperation);
    setRequest(request);
    setUrl(request.url());
    setOpenMode(QIODevice::ReadOnly);

    // Attempt to read from the qrc file corresponding to the request
    loadFile();
}

qint64 ViperNetworkReply::bytesAvailable() const
{
    return m_data.size() - m_offset + QIODevice::bytesAvailable();
}

qint64 ViperNetworkReply::readData(char *data, qint64 maxSize)
{
    qint64 dataLen = m_data.size();
    if (m_offset < dataLen)
    {
        int numBytes = std::min(maxSize, dataLen - m_offset);
        std::memcpy(data, m_data.constData() + m_offset, numBytes);
        m_offset += numBytes;
        return numBytes;
    }
    return -1;
}

void ViperNetworkReply::errorFinished()
{
    emit error(QNetworkReply::ContentNotFoundError);
    emit finished();
    deleteLater();
}

void ViperNetworkReply::loadFile()
{
    QString qrcPath = url().toString();
    qrcPath = qrcPath.right(qrcPath.size() - 8);
    int paramPos = qrcPath.indexOf("?");
    if (paramPos >= 0)
        qrcPath = qrcPath.left(paramPos);

    QFile f(QString(":/%1").arg(qrcPath));
    if (!f.open(QIODevice::ReadOnly))
    {
        QTimer::singleShot(0, this, &ViperNetworkReply::errorFinished);
        return;
    }
    m_data = f.readAll();
    f.close();

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForData(m_data);

    setHeader(QNetworkRequest::ContentTypeHeader, QString("%1; charset=UTF-8").arg(type.name()));
    setHeader(QNetworkRequest::ContentLengthHeader, QVariant(m_data.size()));

    QTimer::singleShot(0, this, &ViperNetworkReply::metaDataChanged);
    QTimer::singleShot(0, this, &ViperNetworkReply::readyRead);
    QTimer::singleShot(0, this, &ViperNetworkReply::finished);
}
