#include "BrowserApplication.h"
#include "FaviconStore.h"
#include "WebHistory.h"
#include "WebPage.h"
#include "WebView.h"

#include <QAction>
#include <QDataStream>

const QString WebHistory::SerializationVersion = QStringLiteral("WebHistory_1.0");

WebHistory::WebHistory(WebPage *parent) :
    QObject(parent),
    m_page(parent),
    m_backAction(nullptr),
    m_forwardAction(nullptr),
    m_entries(),
    m_currentPos(-1),
    m_targetPos(-1),
    m_reloading(false)
{
    if (m_page != nullptr)
    {
        auto reloadAction = m_page->action(WebPage::Reload);
        connect(reloadAction, &QAction::triggered, this, [this](){
            m_targetPos = m_currentPos;
            m_reloading = true;
        });

        connect(m_page, &WebPage::urlChanged, this, &WebHistory::onUrlChanged);
        connect(m_page, &WebPage::loadFinished, this, &WebHistory::onPageLoaded);

        m_backAction = m_page->action(WebPage::Back);
        m_forwardAction = m_page->action(WebPage::Forward);

        connect(m_backAction,    &QAction::changed, this, &WebHistory::historyChanged);
        connect(m_forwardAction, &QAction::changed, this, &WebHistory::historyChanged);
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

    emit historyChanged();
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
    return m_currentPos > 0 || m_backAction->isEnabled();
}

bool WebHistory::canGoForward() const
{
    return m_currentPos + 1 < m_entries.size() || m_forwardAction->isEnabled();
}

void WebHistory::goBack()
{
    if (m_currentPos <= 0 && !m_backAction->isEnabled())
        return;

    m_targetPos = m_currentPos - 1;

    if (m_backAction->isEnabled())
        m_backAction->trigger();
    else if (m_currentPos > 0)
    {
        const WebHistoryEntry &lastEntry = m_entries.at(m_currentPos - 1);
        m_page->load(lastEntry.url);
    }

    //emit historyChanged();
}

void WebHistory::goForward()
{
    if (m_currentPos + 1 == m_entries.size() && !m_forwardAction->isEnabled())
        return;

    m_targetPos = m_currentPos + 1;

    if (m_forwardAction->isEnabled())
        m_forwardAction->trigger();
    else if (m_currentPos + 1 < m_entries.size())
    {
        const WebHistoryEntry &nextEntry = m_entries.at(m_currentPos + 1);
        m_page->load(nextEntry.url);
    }

    //emit historyChanged();
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
            emit historyChanged();
            return;
        }
    }
}

void WebHistory::onPageLoaded(bool /*ok*/)
{
    //if (!ok)
    //    return;

    if (m_reloading)
    {
        m_targetPos = -1;
        return;
    }

    const QUrl url = m_page->url();

    if (m_targetPos >= 0 && m_targetPos < static_cast<int>(m_entries.size()))
    {
        const WebHistoryEntry &entry = m_entries.at(m_targetPos);
        if (entry.url == url)
        {
            m_currentPos = m_targetPos;
            m_targetPos = -1;
            emit historyChanged();
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
    emit historyChanged();
}

void WebHistory::onUrlChanged(const QUrl &url)
{
    WebView *view = qobject_cast<WebView*>(m_page->view());
    if (!view || m_reloading)
        return;

    if (!m_entries.empty() && m_currentPos >= 0)
    {
        const WebHistoryEntry &currentEntry = m_entries.at(m_currentPos);

        if (view->getProgress() == 100)
        {
            if (m_targetPos >= 0 && m_targetPos < m_entries.size())
            {
                const WebHistoryEntry &targetEntry = m_entries.at(m_targetPos);
                if (targetEntry.url == url)
                {
                    m_currentPos = m_targetPos;
                    m_targetPos = -1;
                    emit historyChanged();
                    return;
                }
                else
                {
                    auto it = m_entries.begin() + m_currentPos;
                    if (m_targetPos > m_currentPos)
                        ++it;

                    WebHistoryEntry newEntry(currentEntry.icon, view->getTitle(), url);
                    newEntry.page = m_page;
                    m_entries.insert(it, newEntry);

                    m_currentPos = m_targetPos;
                    m_targetPos = -1;
                    emit historyChanged();
                    return;
                }
            }


            if (url.host() == currentEntry.url.host())
            {
                auto it = m_entries.begin() + m_currentPos + 1;
                WebHistoryEntry newEntry(currentEntry.icon, view->getTitle(), url);
                newEntry.page = m_page;
                m_entries.insert(it, newEntry);
                ++m_currentPos;
            }
        }
    }

    emit historyChanged();
}

