#include "BrowserApplication.h"
#include "BookmarkManager.h"
#include "FaviconStorage.h"
#include "HistoryManager.h"
#include "URLSuggestionWorker.h"

#include <QtConcurrent>
#include <QUrl>

URLSuggestionWorker::URLSuggestionWorker(QObject *parent) :
    QObject(parent),
    m_working(false),
    m_searchTerm(),
    m_suggestionFuture(),
    m_suggestionWatcher(),
    m_suggestions()
{
    connect(&m_suggestionWatcher, &QFutureWatcher<void>::finished, [this](){
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
    m_suggestionWatcher.setFuture(m_suggestionFuture);
}

void URLSuggestionWorker::searchForHits()
{
    m_suggestions.clear();
    m_working.store(true);

    FaviconStorage *faviconStore = sBrowserApplication->getFaviconStorage();

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
            m_suggestions.push_back(suggestion);
            continue;
        }
        QString bookmarkUrl = it->getURL();
        int prefix = bookmarkUrl.indexOf(QLatin1String("://"));
        if (prefix >= 0)
            bookmarkUrl = bookmarkUrl.mid(prefix + 3);
        if (bookmarkUrl.toUpper().contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->getName();
            suggestion.URL = it->getURL();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
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

        if (it->Title.toUpper().contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->Title;
            suggestion.URL = it.key();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            m_suggestions.push_back(suggestion);
            continue;
        }
        QString urlUpper = it.key().toUpper();
        int prefix = urlUpper.indexOf(QLatin1String("://"));
        if (prefix >= 0)
            urlUpper = urlUpper.mid(prefix + 3);

        if (urlUpper.contains(m_searchTerm))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->Title;
            suggestion.URL = it.key();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            m_suggestions.push_back(suggestion);
        }
    }
}
