#ifndef BLOCKEDSCHEMEHANDLER_H
#define BLOCKEDSCHEMEHANDLER_H

#include <QWebEngineUrlSchemeHandler>

class QIODevice;
class QWebEngineUrlRequestJob;

/**
 * @class BlockedSchemeHandler
 * @ingroup AdBlock
 * @brief Handler for blocked URI schemes, usd by the AdBlock system when
 *        redirecting filtered requests
 */
class BlockedSchemeHandler : public QWebEngineUrlSchemeHandler
{
    Q_OBJECT

public:
    /// Constructs the scheme handler with a given parent object
    BlockedSchemeHandler(QObject *parent = nullptr);

    /// Called whenever a request for the blocked scheme is started
    void requestStarted(QWebEngineUrlRequestJob *request) override;
};

#endif // BLOCKEDSCHEMEHANDLER_H
