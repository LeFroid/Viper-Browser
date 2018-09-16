#include "AdBlockManager.h"
#include "BlockedSchemeHandler.h"

#include <QBuffer>
#include <QByteArray>
#include <QUrl>
#include <QWebEngineUrlRequestJob>

BlockedSchemeHandler::BlockedSchemeHandler(QObject *parent) :
    QWebEngineUrlSchemeHandler(parent)
{
}

void BlockedSchemeHandler::requestStarted(QWebEngineUrlRequestJob *request)
{
    AdBlockManager &adBlockMgr = AdBlockManager::instance();

    QString resourceName = request->requestUrl().toString();
    resourceName = resourceName.mid(8);

    QString mimeTypeStr = adBlockMgr.getResourceContentType(resourceName);
    const bool isB64 = mimeTypeStr.contains(QLatin1String(";base64"));
    if (isB64)
        mimeTypeStr = mimeTypeStr.left(mimeTypeStr.indexOf(QLatin1String(";base64")));

    QByteArray mimeType;
    mimeType.append(mimeTypeStr);

    QByteArray resourceData;
    resourceData.append(adBlockMgr.getResource(resourceName));
    if (isB64)
        resourceData = QByteArray::fromBase64(resourceData);

    if (resourceData.isEmpty())
    {
        request->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    QBuffer *buffer = new QBuffer;
    buffer->setData(resourceData);
    if (!buffer->open(QIODevice::ReadOnly))
    {
        delete buffer;
        request->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    connect(request, &QObject::destroyed, buffer, &QBuffer::deleteLater);

    request->reply(mimeType, buffer);
}
