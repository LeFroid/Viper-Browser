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
    m_suggestions(),
    m_differenceHash(0),
    m_searchTermHash(0)
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
    hashSearchTerm();

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

        if (isStringMatch(it->getName().toUpper()))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->getName();
            suggestion.URL = it->getURL();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            suggestion.IsBookmark = true;
            hits.insert(suggestion.URL);
            m_suggestions.push_back(suggestion);
            continue;
        }
        QString bookmarkUrl = it->getURL();
        int prefix = bookmarkUrl.indexOf(QLatin1String("://"));
        if (!searchTermHasScheme && prefix >= 0)
            bookmarkUrl = bookmarkUrl.mid(prefix + 3);
        if (isStringMatch(bookmarkUrl.toUpper()))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->getName();
            suggestion.URL = it->getURL();
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            suggestion.IsBookmark = true;
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

        const QString &url = it.key();
        if (hits.contains(url))
            continue;

        if (isStringMatch(it->Title.toUpper()))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->Title;
            suggestion.URL = url;
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            suggestion.IsBookmark = false;
            m_suggestions.push_back(suggestion);
            continue;
        }
        QString urlUpper = url.toUpper();
        int prefix = urlUpper.indexOf(QLatin1String("://"));
        if (!searchTermHasScheme && prefix >= 0)
            urlUpper = urlUpper.mid(prefix + 3);

        if (isStringMatch(urlUpper))
        {
            URLSuggestion suggestion;
            suggestion.Title = it->Title;
            suggestion.URL = url;
            suggestion.Favicon = faviconStore->getFavicon(suggestion.URL);
            suggestion.IsBookmark = false;
            m_suggestions.push_back(suggestion);
        }
    }

    m_working.store(false);
}

void URLSuggestionWorker::hashSearchTerm()
{
    m_searchTermHash = 0;

    const int needleLength = m_searchTerm.size();
    const QChar *needlePtr = m_searchTerm.constData();

    const quint64 radixLength = 256ULL;
    const quint64 prime = 72057594037927931ULL;

    m_differenceHash = quPow(radixLength, static_cast<quint64>(needleLength - 1)) % prime;

    for (int index = 0; index < needleLength; ++index)
        m_searchTermHash = (radixLength * m_searchTermHash + (needlePtr + index)->toLatin1()) % prime;
}

bool URLSuggestionWorker::isStringMatch(const QString &haystack)
{
    static const quint64 radixLength = 256ULL;
    static const quint64 prime = 72057594037927931ULL;

    const int needleLength = m_searchTerm.size();
    const int haystackLength = haystack.size();

    if (needleLength > haystackLength)
        return false;
    if (needleLength == 0)
        return true;

    const QChar *needlePtr = m_searchTerm.constData();
    const QChar *haystackPtr = haystack.constData();

    int i, j;
    quint64 t = 0;
    quint64 h = 1;

    // The value of h would be "pow(d, M-1)%q"
    for (i = 0; i < needleLength - 1; ++i)
        h = (h * radixLength) % prime;

    // Calculate the hash value of first window of text
    for (i = 0; i < needleLength; ++i)
        t = (radixLength * t + ((haystackPtr + i)->toLatin1())) % prime;

    for (i = 0; i <= haystackLength - needleLength; ++i)
    {
        if (m_searchTermHash == t)
        {
            for (j = 0; j < needleLength; j++)
            {
                if ((haystackPtr + i + j)->toLatin1() != (needlePtr + j)->toLatin1())
                    break;
            }

            if (j == needleLength)
                return true;
        }

        if (i < haystackLength - needleLength)
        {
            t = (radixLength * (t - (h * (haystackPtr + i)->toLatin1()))
                 + (haystackPtr + i + needleLength)->toLatin1()) % prime;
        }
    }

    return false;
}

