#include "HttpRequest.h"

#include <algorithm>

HttpRequest::HttpRequest(const QUrl &url, HttpRequestMethod requestMethod) :
    m_url(url),
    m_requestMethod(requestMethod),
    m_headers(),
    m_postData()
{
}

HttpRequest::HttpRequest(const HttpRequest &other) :
    m_url(other.m_url),
    m_requestMethod(other.m_requestMethod),
    m_headers(other.m_headers),
    m_postData(other.m_postData)
{
}

HttpRequest::HttpRequest(HttpRequest &&other) :
    m_url(other.m_url),
    m_requestMethod(other.m_requestMethod),
    m_headers(std::move(other.m_headers)),
    m_postData(other.m_postData)
{
    other.m_url = QUrl();
    other.m_postData = QByteArray();
}

HttpRequest &HttpRequest::operator=(const HttpRequest &other)
{
    if (&other == this)
        return *this;

    m_url = other.m_url;
    m_requestMethod = other.m_requestMethod;
    m_headers = other.m_headers;
    m_postData = other.m_postData;

    return *this;
}

HttpRequest &HttpRequest::operator=(HttpRequest &&other)
{
    if (&other == this)
        return *this;

    m_url = other.m_url;
    m_requestMethod = other.m_requestMethod;
    m_headers = std::move(other.m_headers);
    m_postData = other.m_postData;

    return *this;
}

HttpRequest::~HttpRequest()
{
}

QByteArray HttpRequest::getHeader(const QByteArray &name) const
{
    auto it = findHeaderByName(name);
    if (it != m_headers.end())
        return it->second;

    return QByteArray();
}

HttpRequestMethod HttpRequest::getMethod() const
{
    return m_requestMethod;
}

QByteArray HttpRequest::getPostData() const
{
    return m_postData;
}

const QUrl &HttpRequest::getUrl() const
{
    return m_url;
}

bool HttpRequest::hasHeader(const QByteArray &name) const
{
    return findHeaderByName(name) != m_headers.end();
}

void HttpRequest::setHeader(const QByteArray &name, const QByteArray &value)
{
    auto it = std::find_if(m_headers.begin(), m_headers.end(), [&name](const std::pair<QByteArray, QByteArray> &item) -> bool {
        return item.first == name;
    });

    if (it != m_headers.end())
    {
        it->second = value;
    }
    else
    {
        m_headers.push_back(std::make_pair(name, value));
    }
}

void HttpRequest::setMethod(HttpRequestMethod method)
{
    m_requestMethod = method;
}

void HttpRequest::setPostData(const QByteArray &postData)
{
    m_postData = postData;
}

void HttpRequest::setUrl(const QUrl &url)
{
    m_url = url;
}

QWebEngineHttpRequest HttpRequest::toWebEngineRequest() const
{
    QWebEngineHttpRequest::Method engineMethod;
    switch (m_requestMethod)
    {
        case HttpRequestMethod::GET:
            engineMethod = QWebEngineHttpRequest::Get;
            break;
        case HttpRequestMethod::POST:
            engineMethod = QWebEngineHttpRequest::Post;
            break;
    }

    QWebEngineHttpRequest engineRequest(m_url, engineMethod);
    engineRequest.setPostData(m_postData);

    for (const auto it : m_headers)
        engineRequest.setHeader(it.first, it.second);

    return engineRequest;
}

void HttpRequest::unsetHeader(const QByteArray &name)
{
    auto it = findHeaderByName(name);
    if (it != m_headers.end())
        m_headers.erase(it);
}

std::vector<std::pair<QByteArray, QByteArray>>::const_iterator HttpRequest::findHeaderByName(const QByteArray &name) const
{
    return std::find_if(m_headers.begin(), m_headers.end(), [&name](const std::pair<QByteArray, QByteArray> &item) -> bool {
        return item.first == name;
    });
}
