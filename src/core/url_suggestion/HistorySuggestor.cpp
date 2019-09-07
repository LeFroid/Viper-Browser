#include "FastHash.h"
#include "FaviconManager.h"
#include "HistoryManager.h"
#include "HistorySuggestor.h"

#include <QRegularExpression>

void HistorySuggestor::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_faviconManager = serviceLocator.getServiceAs<FaviconManager>("FaviconManager");
    m_historyManager = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");
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
    const int maxToSuggest = 75;
    int numSuggested = 0;

    // Strip www prefix from urls when user does not also have this in the search term
    const QRegularExpression prefixExpr = QRegularExpression(QLatin1String("^WWW\\."));
    const bool inputStartsWithWww = searchTerm.size() >= 3 && searchTerm.startsWith(QLatin1String("WWW"));

    // Used to check if certain infrequently visited URLs should be ignored
    const VisitEntry cutoffTime = QDateTime::currentDateTime().addSecs(-259200);

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
            if (!m_historyWords.empty() && searchTermParts.size() > 2 && searchTerm.size() > 4)
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

void HistorySuggestor::timerEvent()
{
    loadHistoryWords();
}
