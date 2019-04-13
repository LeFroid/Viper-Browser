#ifndef BLOCKEDSCHEMEHANDLER_H
#define BLOCKEDSCHEMEHANDLER_H

#include "ServiceLocator.h"
#include <QWebEngineUrlSchemeHandler>

namespace adblock {
    class AdBlockManager;
}

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
    BlockedSchemeHandler(const ViperServiceLocator &serviceLocator, QObject *parent = nullptr);

    /// Called whenever a request for the blocked scheme is started
    void requestStarted(QWebEngineUrlRequestJob *request) override;

private:
    /// Advertisement blocker
    adblock::AdBlockManager *m_adBlockManager;

    /// Service locator
    const ViperServiceLocator &m_serviceLocator;
};

#endif // BLOCKEDSCHEMEHANDLER_H
