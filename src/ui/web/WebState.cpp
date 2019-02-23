#include "BrowserTabWidget.h"
#include "WebHistory.h"
#include "WebState.h"
#include "WebWidget.h"

WebState::WebState(WebWidget *webWidget, BrowserTabWidget *tabWidget) :
    index(0),
    isPinned(false),
    icon(webWidget->getIcon()),
    iconUrl(webWidget->getIconUrl()),
    title(webWidget->getTitle()),
    url(webWidget->url()),
    pageHistory()
{
    if (tabWidget != nullptr)
    {
        index = tabWidget->indexOf(webWidget);
        isPinned = tabWidget->isTabPinned(index);

        if (icon.isNull())
            icon = tabWidget->tabIcon(index);
    }

    if (WebHistory *history = webWidget->getHistory())
        pageHistory = history->save();
}

WebState::WebState(WebState &&other) noexcept :
    index(other.index),
    isPinned(other.isPinned),
    icon(std::move(other.icon)),
    iconUrl(other.iconUrl),
    title(other.title),
    url(other.url),
    pageHistory(std::move(other.pageHistory))
{
}

WebState &WebState::operator=(WebState &&other) noexcept
{
    if (this != &other)
    {
        index = other.index;
        isPinned = other.isPinned;
        icon = std::move(other.icon);
        iconUrl = other.iconUrl;
        title = other.title;
        url = other.url;
        pageHistory = std::move(other.pageHistory);
    }

    return *this;
}

QByteArray WebState::serialize() const
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << index
           << isPinned
           << icon
           << iconUrl
           << title
           << url
           << pageHistory;
    return result;
}

void WebState::deserialize(QByteArray &data)
{
    QDataStream stream(&data, QIODevice::ReadOnly);
    stream  >> index
            >> isPinned
            >> icon
            >> iconUrl
            >> title
            >> url
            >> pageHistory;
}
