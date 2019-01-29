#include "WebHitTestResult.h"
#include "WebPage.h"

WebHitTestResult::WebHitTestResult(WebPage *page, const QString &hitTestScript) :
    m_isEditable(false),
    m_linkUrl(),
    m_mediaUrl(),
    m_mediaType(MediaTypeNone),
    m_selectedText()
{
    if (page)
    {
        QMap<QString, QVariant> resultMap = page->runJavaScriptBlocking(hitTestScript).toMap();
        m_isEditable = resultMap.value(QLatin1String("isEditable")).toBool();
        m_linkUrl = resultMap.value(QLatin1String("linkUrl")).toUrl();
        m_mediaUrl = resultMap.value(QLatin1String("mediaUrl")).toUrl();

        m_mediaType = static_cast<MediaType>(resultMap.value(QLatin1String("mediaType")).toInt());

        m_selectedText = resultMap.value(QLatin1String("selectedText")).toString();

        bool hasSelection = resultMap.value(QLatin1String("hasSelection")).toBool();
        if (hasSelection && m_selectedText.isEmpty())
            m_selectedText = page->selectedText();
    }
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
