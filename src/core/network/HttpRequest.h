#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <utility>
#include <vector>

#include <QByteArray>
#include <QUrl>

#include <QWebEngineHttpRequest>

/// List of HTTP Request types supported by the HttpRequest class
enum class HttpRequestMethod
{
    /// Requests a representation of the specified resource. Requests using GET should only retrieve data.
    GET,

    /// Used to submit an entity to the specified resource, often causing a change in state or side effects on the server.
    POST
};

/**
 * @class HttpRequest
 * @brief Represents an HTTP request that can be passed to a \ref WebWidget and
 *        loaded by its backend web engine.
 */
class HttpRequest
{
public:
    /// Constructs an HTTP Request with a given URL and request method
    HttpRequest(const QUrl &url = QUrl(), HttpRequestMethod requestMethod = HttpRequestMethod::GET);

    /// Copy constructor
    HttpRequest(const HttpRequest &other);

    /// Move constructor
    HttpRequest(HttpRequest &&other);

    /// Copy assignment operator
    HttpRequest &operator =(const HttpRequest &other);

    /// Move assignment operator
    HttpRequest &operator =(HttpRequest &&other);

    /// Destructor
    ~HttpRequest();

    /// Returns the value of the header with the given name, or an empty QByteArray if the header is not set or its value is empty
    QByteArray getHeader(const QByteArray &name) const;

    /// Returns the method of the HTTP request
    HttpRequestMethod getMethod() const;

    /// Returns the raw POST data associated with this request
    QByteArray getPostData() const;

    /// Returns the URL of the request
    const QUrl &getUrl() const;

    /// Returns true if the request has a value set for the given header, false if else
    bool hasHeader(const QByteArray &name) const;

    /// Sets the HTTP header by name to the given value. Any existing header set with the same name will be overwritten
    void setHeader(const QByteArray &name, const QByteArray &value);

    /// Sets the method of this HTTP request
    void setMethod(HttpRequestMethod method);

    /// Sets the raw POST data to be sent with this request if the method is HTTP POST
    void setPostData(const QByteArray &postData);

    /// Sets the URL to be used in the request
    void setUrl(const QUrl &url);

    /// Converts the request to a WebEngine request, returning the QWebEngineHttpRequest equivalent
    QWebEngineHttpRequest toWebEngineRequest() const;

    /// Removes the header with the given name, if it was set
    void unsetHeader(const QByteArray &name);

private:
    /// Searches for a header with the given name, returning an iterator to the name-value pair if found, or an iterator to the end of the container if not found
    std::vector<std::pair<QByteArray, QByteArray>>::const_iterator findHeaderByName(const QByteArray &name) const;

private:
    /// The URL of the request
    QUrl m_url;

    /// The method of the HTTP request
    HttpRequestMethod m_requestMethod;

    /// Stores header name-value pairs to be used in the request
    std::vector<std::pair<QByteArray, QByteArray>> m_headers;

    /// Contains any POST data associated with the request
    QByteArray m_postData;
};

#endif // HTTPREQUEST_H
