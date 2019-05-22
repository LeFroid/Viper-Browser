#ifndef REQUESTINTERCEPTOR_H
#define REQUESTINTERCEPTOR_H

#include "ServiceLocator.h"

#include <memory>
#include <QWebEngineUrlRequestInterceptor>

namespace adblock {
    class AdBlockManager;
}

class Settings;
class WebPage;

/**
 * @class RequestInterceptor 
 * @brief Intercepts network requests before they are made, checking if they need special handlnig.
 */
class RequestInterceptor : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    /// Constructs the request interceptor with an optional parent pointer
    RequestInterceptor(const ViperServiceLocator &serviceLocator, QObject *parent = nullptr);

protected:
    /// Intercepts the given request, potentially blocking it, modifying the header values or redirecting the request
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;

private:
    /// Attempts to fetch the settings and adblock manager services
    void fetchServices();

private:
    /// Pointer to application settings
    Settings *m_settings;

    /// Service locator
    const ViperServiceLocator &m_serviceLocator;

    /// Advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

    /// Parent web page. Only used with QtWebEngine version 5.13 or greater.
    WebPage *m_parentPage;
};

#endif // REQUESTINTERCEPTOR_H 
