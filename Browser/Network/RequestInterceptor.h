#ifndef REQUESTINTERCEPTOR_H
#define REQUESTINTERCEPTOR_H

#include <QWebEngineUrlRequestInterceptor>

/**
 * @class RequestInterceptor 
 * @brief Intercepts network requests before they are made, checking if they need special handlnig.
 */
class RequestInterceptor : public QWebEngineUrlRequestInterceptor
{
    Q_OBJECT

public:
    /// Constructs the request interceptor, optionally setting a parent object
    RequestInterceptor(QObject *parent = nullptr);

protected:
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
};

#endif // REQUESTINTERCEPTOR_H 
