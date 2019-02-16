#include "AdBlockManager.h"
#include "BlockedSchemeHandler.h"

#include <QBuffer>
#include <QByteArray>
#include <QUrl>
#include <QWebEngineUrlRequestJob>

BlockedSchemeHandler::BlockedSchemeHandler(const ViperServiceLocator &serviceLocator, QObject *parent) :
    QWebEngineUrlSchemeHandler(parent),
    m_adBlockManager(nullptr),
    m_serviceLocator(serviceLocator)
{
    m_adBlockManager = serviceLocator.getServiceAs<AdBlockManager>("AdBlockManager");
}

void BlockedSchemeHandler::requestStarted(QWebEngineUrlRequestJob *request)
{
    if (!m_adBlockManager)
        m_adBlockManager = m_serviceLocator.getServiceAs<AdBlockManager>("AdBlockManager");

    QString resourceName = request->requestUrl().toString();
    resourceName = resourceName.mid(8);

    QString mimeTypeStr = m_adBlockManager->getResourceContentType(resourceName);
    const bool isB64 = mimeTypeStr.contains(QLatin1String(";base64"));
    if (isB64)
        mimeTypeStr = mimeTypeStr.left(mimeTypeStr.indexOf(QLatin1String(";base64")));

    QByteArray mimeType;
    mimeType.append(mimeTypeStr);

    QByteArray resourceData;
    resourceData.append(m_adBlockManager->getResource(resourceName));
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
