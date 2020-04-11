#include "ViperSchemeHandler.h"

#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>
#include <QUrl>
#include <QWebEngineUrlRequestJob>

ViperSchemeHandler::ViperSchemeHandler(QObject *parent) :
    QWebEngineUrlSchemeHandler(parent)
{
}

void ViperSchemeHandler::requestStarted(QWebEngineUrlRequestJob *request)
{
    QIODevice *contents = loadFile(request);
    if (!contents)
    {
        request->fail(QWebEngineUrlRequestJob::UrlNotFound);
        return;
    }

    QMimeDatabase db;
    QMimeType type = db.mimeTypeForData(contents);
    QByteArray mimeType = QByteArray::fromStdString(type.name().toStdString());
    if (request && request->requestUrl().toString().endsWith(QLatin1String(".css")))
        mimeType = QByteArray::fromStdString(R"(text/css)");
    request->reply(mimeType, contents);
}

QIODevice *ViperSchemeHandler::loadFile(QWebEngineUrlRequestJob *request)
{
    // Extract file name from URL
    QString qrcPath = request->requestUrl().toString();
    qrcPath = qrcPath.mid(6);
    if (qrcPath.startsWith(QLatin1String("//")))
        qrcPath = qrcPath.mid(2);

    int paramPos = qrcPath.indexOf("?");
    if (paramPos >= 0)
        qrcPath = qrcPath.left(paramPos);

    // Attempt to load the qrc file
    QFile *f = new QFile(QString(":/%1").arg(qrcPath));
    if (!f->open(QIODevice::ReadOnly))
    {
        delete f;
        return nullptr;
    }

    connect(request, &QObject::destroyed, f, &QFile::deleteLater);
    return f;
}
