#ifndef VIPERSCHEMEHANDLER_H
#define VIPERSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>

class QIODevice;
class QWebEngineUrlRequestJob;

/**
 * @class ViperSchemeHandler
 * @brief Implements the viper scheme (wrapper for qrc) for the QtWebEngine backend
 */
class ViperSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    /// Constructs the viper scheme handler with an optional parent
    ViperSchemeHandler(QObject *parent = nullptr);

    /// Called whenever a request for the viper scheme is started
    void requestStarted(QWebEngineUrlRequestJob *request) override;

private:
    /// Loads the qrc file associated with the viper scheme request
    QIODevice *loadFile(QWebEngineUrlRequestJob *request);
};

#endif // VIPERSCHEMEHANDLER_H
