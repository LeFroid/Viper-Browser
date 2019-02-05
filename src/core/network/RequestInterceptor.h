#ifndef REQUESTINTERCEPTOR_H
#define REQUESTINTERCEPTOR_H

#include "ServiceLocator.h"

#include <memory>
#include <QWebEngineUrlRequestInterceptor>

class AdBlockManager;
class Settings;

/**
 * @class RequestInterceptor 
 * @brief Intercepts network requests before they are made, checking if they need special handlnig.
 */
class RequestInterceptor : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    /// Constructs the request interceptor with an optional parent pointer
    RequestInterceptor(ViperServiceLocator &serviceLocator, QObject *parent = nullptr);

    /// Passes a shared pointer to the application settings along to the request interceptor
    void setSettings(Settings *settings);

protected:
    /// Intercepts the given request, potentially blocking it, modifying the header values or redirecting the request
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    /// Pointer to application settings
    Settings *m_settings;

    /// Service locator
    ViperServiceLocator &m_serviceLocator;

    /// AdBlockManager
    AdBlockManager *m_adBlockManager;
};

#endif // REQUESTINTERCEPTOR_H 
