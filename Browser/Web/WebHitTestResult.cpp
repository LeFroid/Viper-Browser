#include "WebHitTestResult.h"

WebHitTestResult::WebHitTestResult(const QMap<QString, QVariant> &scriptResultMap)
{
    m_isEditable = scriptResultMap.value(QLatin1String("isEditable")).toBool();
    m_linkUrl = scriptResultMap.value(QLatin1String("linkUrl")).toUrl();
    m_mediaUrl = scriptResultMap.value(QLatin1String("mediaUrl")).toUrl();

    m_mediaType = static_cast<MediaType>(scriptResultMap.value(QLatin1String("mediaType")).toInt());

    m_selectedText = scriptResultMap.value(QLatin1String("selectedText")).toString();
}

bool WebHitTestResult::isContentEditable() const
{
    return m_isEditable;
}

QUrl WebHitTestResult::linkUrl() const
{
    return m_linkUrl;
}

WebHitTestResult::MediaType WebHitTestResult::mediaType() const
{
    return m_mediaType;
}

QUrl WebHitTestResult::mediaUrl() const
{
    return m_mediaUrl;
}

QString WebHitTestResult::selectedText() const
{
    return m_selectedText;
}
