#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "CommonUtil.h"
#include "FastHash.h"
#include "FaviconManager.h"
#include "HistoryManager.h"
#include "URLSuggestion.h"
#include "URLSuggestionWorker.h"

#include <algorithm>
#include <chrono>

#include <QtConcurrent>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include <QDebug>

// Used to sort results by relevance in the URLSuggestionWorker
bool compareUrlSuggestions(const URLSuggestion &a, const URLSuggestion &b)
{
    // Account for these factors, in order
    // 1) Number of times the URL was previously typed into the URL bar (ignore this check if a and b have never been typed into the bar)
    // 2) Closeness of the url to the user input (ex: search="viper.com", a="vipers-are-cool.com", b="viper.com/faq", choose b)
    // 3) Number of visits to the urls
    // 4) Most recent visit
    // [disabled] 5) Type of match to the search term (ex: the page title vs the URL)
    // 5) Alphabetical ordering

    if (!a.URLTypedCount != !b.URLTypedCount)
        return a.URLTypedCount > b.URLTypedCount;

    if (a.IsHostMatch != b.IsHostMatch)
        return a.IsHostMatch;

    // Special case
    if (!a.PercentMatch != !b.PercentMatch)
        return a.PercentMatch > b.PercentMatch;

    if (a.VisitCount != b.VisitCount)
        return a.VisitCount > b.VisitCount;

    if (a.LastVisit != b.LastVisit)
        return a.LastVisit < b.LastVisit;

    //if (a.Type != b.Type)
    //    return static_cast<int>(a.Type) < static_cast<int>(b.Type);

    return a.URL < b.URL;
}

URLSuggestionWorker::URLSuggestionWorker(QObject *parent) :
    QObject(parent),
    m_working(false),
    m_searchTerm(),
    m_searchWords(),
    m_searchTermHasScheme(false),
    m_suggestionFuture(),
    m_suggestionWatcher(nullptr),
    m_suggestions(),
    m_searchTermWideStr(),
    m_differenceHash(0),
    m_searchTermHash(0),
    m_bookmarkManager(nullptr),
    m_faviconManager(nullptr),
    m_historyManager(nullptr),
    m_mutex(),
    m_cv(),
    m_historyWords(),
    m_historyWordMap()
{
    m_suggestionWatcher = new QFutureWatcher<void>(this);
    connect(m_suggestionWatcher, &QFutureWatcher<void>::finished, [this](){
        emit finishedSearch(m_suggestions);
    });

    // Load word database from history manager every minute
    QTimer *wordTimer = new QTimer(this);
    wordTimer->setSingleShot(false);
    wordTimer->setInterval(std::chrono::milliseconds(1000 * 60));

    connect(wordTimer, &QTimer::timeout, this, &URLSuggestionWorker::onWordTimerTick);
    connect(wordTimer, &QTimer::timeout, wordTimer, static_cast<void(QTimer::*)()>(&QTimer::start));

    wordTimer->start();
}

void URLSuggestionWorker::onWordTimerTick()
{
    if (!m_historyManager)
        return;

    m_historyManager->loadWordDatabase([this](std::map<int, QString> &&wordMap){
        std::lock_guard<std::mutex> lock{m_mutex};
        m_historyWords = std::move(wordMap);
    });

    m_historyManager->loadHistoryWordMapping([this](std::map<int, std::vector<int>> &&historyWordMap){
        std::lock_guard<std::mutex> lock{m_mutex};
        m_historyWordMap = std::move(historyWordMap);
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

    // Remove any http or https prefix from the term, since we do not want to
    // do a string check on URLs and potentially remove an HTTPS match because
    // the user only entered HTTP 
    QRegularExpression httpExpr(QLatin1String("^HTTP(S?)://"));
    m_searchTerm.replace(httpExpr, QString());

    // Split up search term into different words
    m_searchWords = CommonUtil::tokenizePossibleUrl(m_searchTerm);

    m_searchTermHasScheme = (m_searchTerm.startsWith(QLatin1String("FILE"))
            || m_searchTerm.startsWith(QLatin1String("VIPER")));

    hashSearchTerm();

    m_suggestionFuture = QtConcurrent::run(this, &URLSuggestionWorker::searchForHits);
    m_suggestionWatcher->setFuture(m_suggestionFuture);
}

void URLSuggestionWorker::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_bookmarkManager = serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager");
    m_faviconManager  = serviceLocator.getServiceAs<FaviconManager>("FaviconManager");
    m_historyManager  = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");

    onWordTimerTick();
}

void URLSuggestionWorker::searchForHits()
{
    m_working.store(true);
    m_suggestions.clear();

    if (!m_bookmarkManager || !m_faviconManager || !m_historyManager)
    {
        m_working.store(false);
        return;
    }

    // Set upper bound on number of suggestions for each type of item being checked
    const int maxSuggestedBookmarks = 20, maxSuggestedHistory = 75;
    int numSuggestedBookmarks = 0, numSuggestedHistory = 0;

    // Store urls being suggested in a set to avoid duplication when checking different data sources
    QSet<QString> hits;

    const bool inputStartsWithWww = m_searchTerm.size() >= 3 && m_searchTerm.startsWith(QLatin1String("WWW"));

    for (const auto &it : *m_bookmarkManager)
    {
        if (!m_working.load())
            return;

        if (it->getType() != BookmarkNode::Bookmark)
            continue;

        const QString url = it->getURL().toString();
        MatchType matchType = getMatchType(it->getName().toUpper(), url.toUpper(), it->getShortcut().toUpper());
        if (matchType != MatchType::None)
        {
            URLSuggestion suggestion { it, m_historyManager->getEntry(it->getURL()), matchType };

            QString suggestionHost = it->getURL().host().toUpper();
            if (!inputStartsWithWww)
                suggestionHost = suggestionHost.replace(QRegularExpression(QLatin1String("^WWW\\.")), QString());
            suggestion.IsHostMatch = suggestionHost.startsWith(m_searchTerm);

            hits.insert(suggestion.URL);
            m_suggestions.push_back(suggestion);

            if (++numSuggestedBookmarks == maxSuggestedBookmarks)
                break;
        }
    }

    if (!m_working.load())
        return;

    // Used to check if certain infrequently visited URLs should be ignored
    const VisitEntry cutoffTime = QDateTime::currentDateTime().addSecs(-259200);
    {
    std::lock_guard<std::mutex> lock{m_mutex};
    for (const auto &it : *m_historyManager)
    {
        if (!m_working.load())
            return;

        const URLRecord &record = it.second;
        const QString &url = record.getUrl().toString();
        if (hits.contains(url))
            continue;

        const QUrl &urlObj = record.getUrl();

        // Rule out records that don't match any of these criteria
        if (record.getUrlTypedCount() < 1
                && record.getNumVisits() < 4
                && record.getLastVisit() < cutoffTime)
            continue;

        MatchType matchType = MatchType::None;
        int percentWordScore = 0;
        if (!m_historyWords.empty() && m_searchWords.size() > 2 && m_searchTerm.size() > 4)
        {
            int wordMatches = 0;
            float score = 0.0f;

            std::vector<QString> historyWords;
            auto wordIt = m_historyWordMap.find(record.getVisitId());
            if (wordIt != m_historyWordMap.end())
            {
                std::vector<int> &wordIds = wordIt->second;
                for (int id : wordIds)
                    historyWords.push_back(m_historyWords[id]);
            }

            for (const QString &histWord : historyWords)
            {
                const int histWordLen = histWord.size();

                for (const QString &searchWord : m_searchWords)
                {
                    const int offset = histWord.indexOf(searchWord);
                    if (offset >= 0)
                    {
                        score += static_cast<float>(searchWord.size()) * (static_cast<float>(histWordLen - offset) / histWordLen);
                        ++wordMatches;
                    }
                    else if (searchWord.contains(histWord))
                        ++wordMatches;
                }
            }

            const float scoreRatio = score / static_cast<float>(url.size());
            if (wordMatches + 1 >= m_searchWords.size() && scoreRatio >= 0.275f)
            {
                //qDebug() << "Search term: " << m_searchTerm << "Score: " << score << ", Ratio: " << (score / static_cast<float>(url.size()))
                //         << " URL: " << url << "Title: " << record.getTitle() << "Search Words: " << m_searchWords;
                matchType = MatchType::SearchWords;
                percentWordScore = static_cast<int>(100.0f * scoreRatio);
            }
            else if (isStringMatch(record.getTitle().toUpper()))
                matchType = MatchType::Title;
        }

        if (matchType == MatchType::None)
        {
            const int urlIndex = url.toUpper().mid(7).indexOf(m_searchTerm);
            if ((urlIndex >= 0 && urlIndex < 10)
                    || (urlIndex > 0 && (static_cast<float>(m_searchTerm.size()) / static_cast<float>(url.size())) >= 0.25f))
                matchType = MatchType::URL;
            else if (isStringMatch(record.getTitle().toUpper()))
                matchType = MatchType::Title;
        }

        //else
        //    matchType = getMatchType(record.getTitle().toUpper(), url.toUpper());

        if (matchType != MatchType::None)
        {
            URLSuggestion suggestion { record, m_faviconManager->getFavicon(urlObj), matchType };

            QString suggestionHost = urlObj.host().toUpper();
            if (!inputStartsWithWww)
                suggestionHost = suggestionHost.replace(QRegularExpression(QLatin1String("^WWW\\.")), QString());
            suggestion.IsHostMatch = m_searchTerm.startsWith(suggestionHost);

            if (matchType == MatchType::SearchWords)
                suggestion.PercentMatch = percentWordScore;

            m_suggestions.push_back(suggestion);
            hits.insert(suggestion.URL);

            if (++numSuggestedHistory == maxSuggestedHistory)
                break;
        }
    }
    }

    std::sort(m_suggestions.begin(), m_suggestions.end(), compareUrlSuggestions);
    if (m_suggestions.size() > 35)
        m_suggestions.erase(m_suggestions.begin() + 35, m_suggestions.end());

    m_working.store(false);
}

MatchType URLSuggestionWorker::getMatchType(const QString &title, const QString &url, const QString &shortcut)
{
    if (!shortcut.isEmpty() && m_searchTerm.startsWith(shortcut))
        return MatchType::Shortcut;

    // Special case for small search terms
    if (m_searchTerm.size() < 5)
        return getMatchTypeForSmallSearchTerm(title, url);

    if (url.indexOf(m_searchTerm) >= 0
            && (static_cast<float>(m_searchTerm.size()) / static_cast<float>(url.size())) >= 0.325f)
            return MatchType::URL;

    const int numWords = m_searchWords.size();
    if (numWords > 1)
    {
        int numMatchingWords = 0;
        for (const QString &word : m_searchWords)
        {
            if (word.size() > 2 && title.contains(word, Qt::CaseSensitive))
                numMatchingWords++;
                //return MatchType::SearchWords;
        }

        if (static_cast<float>(numMatchingWords) / static_cast<float>(numWords) >= 0.5f)
            return MatchType::SearchWords;
    }

    if (isStringMatch(title))
        return MatchType::Title;

    return isStringMatch(url) ? MatchType::URL : MatchType::None;
}

MatchType URLSuggestionWorker::getMatchTypeForSmallSearchTerm(const QString &title, const QString &url)
{
    QStringList nameParts = title.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (nameParts.empty())
        nameParts.push_back(title);

    for (const QString &part : nameParts)
    {
        if (part.startsWith(m_searchTerm))
            return MatchType::Title;
    }

    QString urlMutable = url;
    const int prefix = url.indexOf(QLatin1String("://"));
    if (!m_searchTermHasScheme && prefix >= 0)
        urlMutable = urlMutable.mid(prefix + 3);

    if (m_searchTerm.at(0) != QLatin1Char('W') && urlMutable.startsWith(QLatin1String("WWW.")))
        urlMutable = urlMutable.mid(4);

    return urlMutable.startsWith(m_searchTerm) ? MatchType::URL : MatchType::None;
}

bool URLSuggestionWorker::isEntryMatch(const QString &title, const QString &url, const QString &shortcut)
{
    if (!shortcut.isEmpty() && m_searchTerm.startsWith(shortcut))
        return true;

    // Special case for small search terms
    if (m_searchTerm.size() < 5)
        return isMatchForSmallSearchTerm(title, url);

    if (isStringMatch(title))
        return true;

    if (m_searchWords.size() > 1)
    {
        for (const QString &word : m_searchWords)
        {
            if (word.size() > 2 && title.contains(word, Qt::CaseSensitive))
                return true;
        }
    }

    return isStringMatch(url);
}

bool URLSuggestionWorker::isMatchForSmallSearchTerm(const QString &title, const QString &url)
{
    QStringList nameParts = title.split(QLatin1Char(' '), QString::SkipEmptyParts);
    if (nameParts.empty())
        nameParts.push_back(title);

    for (const QString &part : nameParts)
    {
        if (part.startsWith(m_searchTerm))
            return true;
    }

    const int prefix = url.indexOf(QLatin1String("://"));
    QString urlMutable = url;
    if (!m_searchTermHasScheme && prefix >= 0)
        urlMutable = urlMutable.mid(prefix + 3);

    if (m_searchTerm.at(0) != QLatin1Char('W') && urlMutable.startsWith(QLatin1String("WWW.")))
        urlMutable = urlMutable.mid(4);

    return urlMutable.startsWith(m_searchTerm);
}

void URLSuggestionWorker::hashSearchTerm()
{
    m_searchTermWideStr = m_searchTerm.toStdWString();
    m_differenceHash = FastHash::getDifferenceHash(static_cast<quint64>(m_searchTerm.size()));
    m_searchTermHash = FastHash::getNeedleHash(m_searchTermWideStr);
}

bool URLSuggestionWorker::isStringMatch(const QString &haystack)
{
    std::wstring haystackWideStr = haystack.toStdWString();
    return FastHash::isMatch(m_searchTermWideStr, haystackWideStr, m_searchTermHash, m_differenceHash);
}
