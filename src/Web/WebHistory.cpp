#include "BrowserApplication.h"
#include "FaviconStore.h"
#include "WebHistory.h"
#include "WebPage.h"

#include <QAction>
#include <QDataStream>

const QString WebHistory::SerializationVersion = QStringLiteral("WebHistory_1.0");

WebHistory::WebHistory(WebPage *parent) :
    QObject(parent),
    m_page(parent),
    m_entries(),
    m_currentPos(-1),
    m_targetPos(-1)
{
    if (m_page != nullptr)
    {
        connect(m_page, &WebPage::loadFinished, this, &WebHistory::onPageLoaded);

        QAction *pageAction = m_page->action(WebPage::Back);
        connect(pageAction, &QAction::triggered, [this](){
            m_targetPos = m_currentPos - 1;
        });
        pageAction = m_page->action(WebPage::Forward);
        connect(pageAction, &QAction::triggered, [this](){
            m_targetPos = m_currentPos + 1;
        });
    }
}

WebHistory::~WebHistory()
{
}

void WebHistory::load(QByteArray &data)
{
    m_currentPos = -1;
    m_targetPos = -1;
    m_entries.clear();

    quint64 entriesLen =  0;
    qint64 position = 0;
    QString version;

    QDataStream stream(&data, QIODevice::ReadOnly);
    stream >> version;
    if (version.compare(SerializationVersion) != 0)
        return;

    stream >> entriesLen
           >> position;

    for (quint64 i = 0; i < entriesLen; ++i)
    {
        WebHistoryEntry entry;
        stream  >> entry.icon
                >> entry.title
                >> entry.url
                >> entry.visitTime;
        entry.page = m_page;
        m_entries.push_back(entry);
    }

    m_currentPos = static_cast<int>(position);
    m_targetPos = m_currentPos;
}

QByteArray WebHistory::save() const
{
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << SerializationVersion
           << static_cast<quint64>(m_entries.size())
           << static_cast<qint64>(m_currentPos);

    //for each entry, save its icon, title, url and visit time
    for (const WebHistoryEntry &entry : m_entries)
    {
        stream << entry.icon
               << entry.title
               << entry.url
               << entry.visitTime;
    }

    return result;
}

std::vector<WebHistoryEntry> WebHistory::getBackEntries(int maxEntries) const
{
    const bool respectLimit = maxEntries >= 0;

    std::vector<WebHistoryEntry> result;
    if (maxEntries == 0 || m_currentPos <= 0)
        return result;

    int startPos = 0;
    if (respectLimit && maxEntries < m_currentPos)
        startPos = m_currentPos - maxEntries;

    auto startItr = m_entries.begin() + startPos;
    auto endItr = m_entries.begin() + m_currentPos;
    while (startItr != endItr)
    {
        result.push_back(*startItr);
        ++startItr;
    }

    return result;
}

std::vector<WebHistoryEntry> WebHistory::getForwardEntries(int maxEntries) const
{
    const bool respectLimit = maxEntries >= 0;

    std::vector<WebHistoryEntry> result;
    if (maxEntries == 0 || m_currentPos + 1 == m_entries.size())
        return result;

    int entryCount = m_entries.size() - m_currentPos - 1;
    if (respectLimit && maxEntries < entryCount)
        entryCount = maxEntries;

    auto it = m_entries.begin() + m_currentPos + 1;
    for (int i = 0; i < entryCount; ++i)
    {
        result.push_back(*it);
        ++it;
    }

    return result;
}

bool WebHistory::canGoBack() const
{
    return m_currentPos > 0;
}

bool WebHistory::canGoForward() const
{
    return m_currentPos + 1 < m_entries.size();
}

void WebHistory::goBack()
{
    if (m_currentPos <= 0)
        return;

    const WebHistoryEntry &entry = m_entries.at(m_currentPos - 1);
    m_targetPos = m_currentPos - 1;
    m_page->load(entry.url);
    //m_currentPos -= 1;
}

void WebHistory::goForward()
{
    if (m_currentPos + 1 == m_entries.size())
        return;

    const WebHistoryEntry &entry = m_entries.at(m_currentPos + 1);
    m_targetPos = m_currentPos + 1;
    m_page->load(entry.url);
    //m_currentPos += 1;
}

void WebHistory::goToEntry(const WebHistoryEntry &entry)
{
    if (entry.page != m_page)
        return;

    for (int i = 0; i < m_entries.size(); ++i)
    {
        const WebHistoryEntry &currentEntry = m_entries.at(i);
        if (currentEntry.visitTime == entry.visitTime
                && currentEntry.url == entry.url
                && i != m_currentPos)
        {
            m_targetPos = i;
            m_page->load(entry.url);
            return;
        }
    }
}

void WebHistory::reload()
{
    m_targetPos = m_currentPos;
    m_page->triggerAction(WebPage::Reload);
}

void WebHistory::onPageLoaded(bool ok)
{
    if (!ok)
        return;

    const QUrl url = m_page->url();

    if (m_targetPos >= 0 && m_targetPos < static_cast<int>(m_entries.size()))
    {
        const WebHistoryEntry &entry = m_entries.at(m_targetPos);
        if (entry.url == url)
        {
            m_currentPos = m_targetPos;
            m_targetPos = -1;
            return;
        }
    }

    const QString title = m_page->title();
    QIcon favicon = m_page->icon();
    if (favicon.isNull())
        favicon = sBrowserApplication->getFaviconStore()->getFavicon(url, false);

    if (m_currentPos + 1 < m_entries.size())
        m_entries.erase(m_entries.begin() + m_currentPos + 1, m_entries.end());

    WebHistoryEntry entry(favicon, title, url);
    entry.page = m_page;
    m_entries.push_back(entry);

    ++m_currentPos;
}
