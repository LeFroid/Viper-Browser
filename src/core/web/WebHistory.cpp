#include "FaviconManager.h"
#include "WebHistory.h"
#include "WebPage.h"
#include "WebView.h"

#include <QAction>
#include <QDataStream>

const QString WebHistory::SerializationVersion = QStringLiteral("WebHistory_2.0");

WebHistoryEntry::WebHistoryEntry(FaviconManager *faviconManager, const WebHistoryEntryImpl &impl) :
    icon(),
    title(impl.title()),
    url(impl.url()),
    visitTime(impl.lastVisited()),
    impl(impl)
{
    if (faviconManager)
    {
        QUrl iconUrl = impl.iconUrl();
        if (iconUrl.isEmpty() || !iconUrl.isValid())
            iconUrl = url;

        icon = faviconManager->getFavicon(iconUrl);
    }
}

WebHistory::WebHistory(const ViperServiceLocator &serviceLocator, WebPage *parent) :
    QObject(parent),
    m_faviconManager(serviceLocator.getServiceAs<FaviconManager>("FaviconManager")),
    m_page(parent),
    m_impl(nullptr)
{
    if (m_page != nullptr)
    {
        m_impl = m_page->history();
        connect(m_page, &WebPage::destroyed, [this](){
            m_impl = nullptr;
        });

        connect(m_page, &WebPage::loadFinished, this, &WebHistory::historyChanged);
    }
}

WebHistory::~WebHistory()
{
}

void WebHistory::load(QByteArray &data)
{
    QString version;

    QDataStream stream(&data, QIODeviceBase::ReadOnly);
    stream >> version;
    if (version.compare(SerializationVersion) != 0)
        return;

    stream >> *m_impl;

    emit historyChanged();
}

QByteArray WebHistory::save() const
{
    QByteArray result;
    QDataStream stream(&result, QIODeviceBase::WriteOnly);
    stream << SerializationVersion
           << *m_impl;

    return result;
}

std::vector<WebHistoryEntry> WebHistory::getBackEntries(int maxEntries) const
{
    std::vector<WebHistoryEntry> result;
    if (!m_impl)
        return result;

    auto backEntries = maxEntries > 0 ? m_impl->backItems(maxEntries) : m_impl->backItems(10);
    for (const auto &entry : backEntries)
        result.push_back(WebHistoryEntry(m_faviconManager, entry));

    return result;
}

std::vector<WebHistoryEntry> WebHistory::getForwardEntries(int maxEntries) const
{
    std::vector<WebHistoryEntry> result;
    if (!m_impl)
        return result;

    auto forwardEntries = maxEntries > 0 ? m_impl->forwardItems(maxEntries) : m_impl->forwardItems(10);
    for (const auto &entry : forwardEntries)
        result.push_back(WebHistoryEntry(m_faviconManager, entry));

    return result;
}

bool WebHistory::canGoBack() const
{
    if (!m_impl)
        return false;

    return m_impl->canGoBack();
}

bool WebHistory::canGoForward() const
{
    if (!m_impl)
        return false;

    return m_impl->canGoForward();
}

void WebHistory::goBack()
{
    if (m_impl)
        m_impl->back();
}

void WebHistory::goForward()
{
    if (m_impl)
        m_impl->forward();
}

void WebHistory::goToEntry(const WebHistoryEntry &entry)
{
    if (m_impl)
        m_impl->goToItem(entry.impl);
}
