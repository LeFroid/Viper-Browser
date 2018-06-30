#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "FaviconStorage.h"
#include "HistoryManager.h"
#include "URLSuggestionWorker.h"

#include <QtConcurrent>
#include <QSet>
#include <QUrl>

URLSuggestionWorker::URLSuggestionWorker(QObject *parent) :
    QObject(parent),
    m_working(false),
    m_searchTerm(),
    m_suggestionFuture(),
    m_suggestionWatcher(nullptr),
    m_suggestions()
{
    m_suggestionWatcher = new QFutureWatcher<void>(this);
    connect(m_suggestionWatcher, &QFutureWatcher<void>::finished, [this](){
        emit finishedSearch(m_suggestions);
    });
}

void URLSuggestionWorker::findSuggestionsFor(const QString &text)
{
    if (m_working)
    {
        m_working.store(false);
        m_suggestionFuture.cancel();
        m_suggestionFuture.waitForFinished();
    }

    m_searchTerm = text.toUpper();
    m_suggestionFuture = QtConcurrent::run(this, &URLSuggestionWorker::searchForHits);
    m_suggestionWatcher->setFuture(m_suggestionFuture);
}

void URLSuggestionWorker::searchForHits()
{
    m_working.store(true);
    m_suggestions.clear();

    const bool searchTermHasScheme = m_searchTerm.startsWith(QLatin1String("HTTP"))
            || m_searchTerm.startsWith(QLatin1String("FILE"));

    FaviconStorage *faviconStore = sBrowserApplication->getFaviconStorage();

    // Store urls being suggested in a set to avoid duplication when checking different data sources
    QSet<QString> hits;

    BookmarkManager *bookmarkMgr = sBrowserApplication->getBookmarkManager();
    for (auto it : *bookmarkMgr)
    {
        if (!m_working.load())
            return;

        if (it->getType() != BookmarkNode::Bookmark)
            continue;

        if (it->getName().toUpper().contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->getName();
            suggestion.URL = it->getURL();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            hits.insert(suggestion.URL);
            m_suggestions.push_back(suggestion);
            continue;
        }
        QString bookmarkUrl = it->getURL();
        int prefix = bookmarkUrl.indexOf(QLatin1String("://"));
        if (!searchTermHasScheme && prefix >= 0)
            bookmarkUrl = bookmarkUrl.mid(prefix + 3);
        if (bookmarkUrl.toUpper().contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->getName();
            suggestion.URL = it->getURL();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            hits.insert(suggestion.URL);
            m_suggestions.push_back(suggestion);
        }
    }

    if (!m_working.load())
        return;

    HistoryManager *historyMgr = sBrowserApplication->getHistoryManager();
    for (auto it = historyMgr->getHistIterBegin(); it != historyMgr->getHistIterEnd(); ++it)
    {
        if (!m_working.load())
            return;

        QString url = it.key();

        int queryPos = url.indexOf(QLatin1Char('?'));
        if (queryPos > 0)
            url = url.left(queryPos);

        if (hits.contains(url))
            continue;

        if (it->Title.toUpper().contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->Title;
            suggestion.URL = url;
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            m_suggestions.push_back(suggestion);
            continue;
        }
        QString urlUpper = url.toUpper();
        int prefix = urlUpper.indexOf(QLatin1String("://"));
        if (!searchTermHasScheme && prefix >= 0)
            urlUpper = urlUpper.mid(prefix + 3);

        if (urlUpper.contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->Title;
            suggestion.URL = url;
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            m_suggestions.push_back(suggestion);
        }
    }

    m_working.store(false);
}
