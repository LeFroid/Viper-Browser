#include "BookmarkManager.h"
#include "FastHash.h"
#include "FaviconManager.h"
#include "HistoryManager.h"
#include "HistorySuggestor.h"

#include <algorithm>
#include <QRegularExpression>

#include <QDebug>

void HistorySuggestor::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_bookmarkManager = serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager");
    m_faviconManager  = serviceLocator.getServiceAs<FaviconManager>("FaviconManager");
    m_historyManager  = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");
}

std::vector<URLSuggestion> HistorySuggestor::getSuggestions(const std::atomic_bool &working,
                                                            const QString &searchTerm,
                                                            const QStringList &searchTermParts,
                                                            const FastHashParameters &hashParams)
{
    std::vector<URLSuggestion> result;

    if (!m_faviconManager || !m_historyManager)
        return result;

    // Upper bound on number of results
    const int maxToSuggest = 50;
    int numSuggested = 0;

    // Strip www prefix from urls when user does not also have this in the search term
    const QRegularExpression prefixExpr = QRegularExpression(QLatin1String("^WWW\\."));
    const bool inputStartsWithWww = searchTerm.size() >= 3 && searchTerm.startsWith(QLatin1String("WWW"));

    // Used to check if certain infrequently visited URLs should be ignored (5 days)
    const VisitEntry cutoffTime = QDateTime::currentDateTime().addSecs(-432000);

    // Factored into an entry's score
    const qint64 currentTime = QDateTime::currentDateTime().toSecsSinceEpoch();

    // Sort search words by their string length, in descending order
    std::vector<QString> searchWords;
    searchWords.reserve(static_cast<size_t>(searchTermParts.size()));
    for (const QString &word : searchTermParts)
        searchWords.push_back(word);

    std::sort(searchWords.begin(), searchWords.end(), [](const QString &a, const QString &b) -> bool {
        return a.length() > b.length();
    });

    {   
        std::lock_guard<std::mutex> lock{m_mutex};
        for (const auto &it : *m_historyManager)
        {
            if (!working.load())
                return result;

            const URLRecord &record = it.second;
            const QString &url = record.getUrl().toString();
            const QUrl &urlObj = record.getUrl();

            // Rule out records that don't match any of these criteria
            if (record.getUrlTypedCount() < 1
                    && record.getNumVisits() < 4
                    && record.getLastVisit() < cutoffTime)
                continue;

            MatchType matchType = MatchType::None;
            int percentWordScore = 0;

            ///TODO: move inner portion of if statement into separate method
            if (!m_historyWords.empty() && searchTermParts.size() >= 2 && searchTerm.size() > 4)
            {
                // int wordMatches = 0;
                float score = 0.0f;

                bool skipCurrentEntry = false;

                std::vector<QString> historyWords = getHistoryEntryWords(record.getVisitId());
                for (const QString &searchWord : searchWords)
                {
                    auto wordIt = std::find_if(historyWords.begin(), historyWords.end(), [&searchWord](const QString &historyWord) -> bool {
                       return historyWord.contains(searchWord);
                    });

                    if (wordIt == historyWords.end())
                    {
                        skipCurrentEntry = true;
                        break;
                    }
                    else
                    {
                        const int histWordLen = wordIt->size();
                        const int offset = wordIt->indexOf(searchWord);
                        score += static_cast<float>(searchWord.size()) * (static_cast<float>(histWordLen - offset) / histWordLen);
                    }
                }

                if (skipCurrentEntry)
                    continue;

                matchType = MatchType::SearchWords;
                const float scoreTermRatio = score / static_cast<float>(searchTerm.size());
                const float scoreUrlRatio = score / static_cast<float>(url.size());
                // Grant up to additional 15 points for non-direct factors
                // These are:
                //   (1) Recency of last visit: Within last week? + 5pts
                //   (2) Number of total visits: >= 25 visits ? + 10 pts, otherwise, (visitCount/25.0) * 10 for partial credit
                float bonus = (currentTime - record.getLastVisit().toSecsSinceEpoch()) <= 604800LL ? 5.0f : 0.0f;
                if (record.getNumVisits() >= 25)
                    bonus += 10.0f;
                else
                    bonus += 10.0f * (static_cast<float>(record.getNumVisits()) / 25.0f);

                percentWordScore = static_cast<int>((50.0f * scoreUrlRatio) + (50.0f * scoreTermRatio));

                // qDebug() << "Search term: " << searchTerm << ", URL: " << url << ", Percent Score: " << percentWordScore;

                // Previous code:
                /*
                for (const QString &histWord : historyWords)
                {
                    const int histWordLen = histWord.size();

                    for (const QString &searchWord : searchTermParts)
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
                if (wordMatches + 1 >= searchTermParts.size() && scoreRatio >= 0.275f)
                {
                    matchType = MatchType::SearchWords;
                    percentWordScore = static_cast<int>(100.0f * scoreRatio);
                }
                else if (FastHash::isMatch(hashParams.needle, record.getTitle().toUpper().toStdWString(), hashParams.needleHash, hashParams.differenceHash))
                    matchType = MatchType::Title;
                */
            }

            if (matchType == MatchType::None)
            {
                const int urlIndex = url.toUpper().mid(7).indexOf(searchTerm);
                if ((urlIndex >= 0 && urlIndex < 10)
                        || (urlIndex > 0 && (static_cast<float>(searchTerm.size()) / static_cast<float>(url.size())) >= 0.25f))
                    matchType = MatchType::URL;
                else if (FastHash::isMatch(hashParams.needle, record.getTitle().toUpper().toStdWString(), hashParams.needleHash, hashParams.differenceHash))
                    matchType = MatchType::Title;
            }

            if (matchType != MatchType::None)
            {
                URLSuggestion suggestion { record, m_faviconManager->getFavicon(urlObj), matchType };

                QString suggestionHost = urlObj.host().toUpper();
                if (!inputStartsWithWww)
                    suggestionHost = suggestionHost.replace(prefixExpr, QString());
                suggestion.IsHostMatch = searchTerm.startsWith(suggestionHost);

                if (matchType == MatchType::SearchWords)
                    suggestion.PercentMatch = percentWordScore;

                if (m_bookmarkManager)
                    suggestion.IsBookmark = m_bookmarkManager->isBookmarked(urlObj);

                result.push_back(suggestion);

                if (++numSuggested >= maxToSuggest)
                    return result;
            }
        }
    }

    return result;
}

void HistorySuggestor::loadHistoryWords()
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

std::vector<QString> HistorySuggestor::getHistoryEntryWords(int historyId)
{
    std::vector<QString> historyWords;
    auto wordIt = m_historyWordMap.find(historyId);
    if (wordIt != m_historyWordMap.end())
    {
        std::vector<int> &wordIds = wordIt->second;
        for (int id : wordIds)
            historyWords.push_back(m_historyWords[id]);
    }
    return historyWords;
}

void HistorySuggestor::timerEvent()
{
    loadHistoryWords();
}
