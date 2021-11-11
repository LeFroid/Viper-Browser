#include "BookmarkManager.h"
#include "FastHash.h"
#include "FaviconManager.h"
#include "HistorySuggestor.h"
#include "Settings.h"
#include "URLRecord.h"

#include "SQLiteWrapper.h"

#include <algorithm>
#include <numeric>
#include <thread>

#include <QDateTime>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>

#include <QDebug>

void HistorySuggestor::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_bookmarkManager = serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager");
    m_faviconManager  = serviceLocator.getServiceAs<FaviconManager>("FaviconManager");

    if (Settings *settings = serviceLocator.getServiceAs<Settings>("Settings"))
    {
        m_historyDatabaseFile = settings->getPathValue(BrowserSetting::HistoryPath);
    }
}

void HistorySuggestor::setHistoryFile(const QString &historyDbFile)
{
    m_historyDatabaseFile = historyDbFile;
}

std::vector<URLSuggestion> HistorySuggestor::getSuggestions(const std::atomic_bool &working,
                                                            const QString &searchTerm,
                                                            const QStringList &searchTermParts,
                                                            const FastHashParameters &/*hashParams*/)
{
    std::vector<URLSuggestion> result;

    if (!m_faviconManager || m_historyDatabaseFile.isEmpty())
        return result;

    if (!m_historyDb)
        setupConnection();

    if (!m_historyDb || !m_historyDb->isValid())
        return result;

    const QString fullTermParam = QString("%%1%").arg(searchTerm);

    sqlite::PreparedStatement &stmt = m_statements.at(Statement::SearchByWholeInput);
    stmt.reset();
    stmt << fullTermParam
         << fullTermParam;
    if (stmt.execute())
    {
        auto fullTermResult = getSuggestionsFromQuery(working, searchTerm, MatchType::URL, stmt);
        result.insert(result.end(), std::make_move_iterator(fullTermResult.begin()), std::make_move_iterator(fullTermResult.end()));
        if (!working.load())
            return result;
    }

    if (searchTermParts.size() == 1)
        return result;

    // Sort search words by their string length, in descending order
    std::vector<QString> searchWords;
    searchWords.reserve(static_cast<size_t>(searchTermParts.size()));
    for (const QString &word : searchTermParts)
        searchWords.push_back(word);

    std::sort(searchWords.begin(), searchWords.end(), [](const QString &a, const QString &b) -> bool {
        return a.length() > b.length();
    });

    sqlite::PreparedStatement &stmtWords = m_statements.at(Statement::SearchBySingleWord);

    for (const QString &word : searchWords)
    {
        if (!working.load())
            return result;

        QString wordTemp = word.size() < 4 ? QString("%%1%") : QString("%1%");
        wordTemp = wordTemp.arg(word.trimmed().toUpper());
        const std::string wordStdBinding = wordTemp.toStdString();

        stmtWords.reset();
        stmtWords << wordStdBinding;

        if (!stmtWords.execute())
            continue;

        auto wordQueryResult = getSuggestionsFromQuery(working, searchTerm, MatchType::SearchWords, stmtWords);
        for (auto& suggestion : wordQueryResult)
        {
            auto match = std::find_if(result.begin(), result.end(), [&suggestion](const URLSuggestion &other){
                return other.HistoryId == suggestion.HistoryId;
            });

            if (match == result.end())
                result.emplace_back(std::move(suggestion));
        }
    }

    return result;
}
    /*
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
            }
    */

std::vector<URLSuggestion> HistorySuggestor::getSuggestionsFromQuery(const std::atomic_bool &working,
                                                                     const QString &searchTerm,
                                                                     MatchType queryMatchType,
                                                                     sqlite::PreparedStatement &query)
{
    constexpr int maxToSuggest = 25;
    int numSuggested = 0;

    // Strip www prefix from urls when user does not also have this in the search term
    const QRegularExpression prefixExpr = QRegularExpression(QLatin1String("^WWW\\."));
    const bool inputStartsWithWww = searchTerm.size() >= 3 && searchTerm.startsWith(QLatin1String("WWW"));

    const VisitEntry cutoffTime = QDateTime::currentDateTime().addSecs(-1814400);

    std::vector<URLSuggestion> result;

    while (query.next())
    {
        if (!working.load())
            return result;

        HistoryEntry entry;
        query >> entry;

        if (entry.URLTypedCount < 1
                && entry.NumVisits < 4
                && entry.LastVisit < cutoffTime)
            continue;

        std::vector<VisitEntry> emptyVisits;
        URLRecord urlRecord{ std::move(entry), std::move(emptyVisits) };

        URLSuggestion suggestion { urlRecord, m_faviconManager->getFavicon(urlRecord.getUrl()), queryMatchType };

        QString suggestionHost = urlRecord.getUrl().host().toUpper();
        if (!inputStartsWithWww)
            suggestionHost = suggestionHost.replace(prefixExpr, QString());
        suggestion.IsHostMatch = searchTerm.startsWith(suggestionHost);

        result.push_back(suggestion);
        if (++numSuggested >= maxToSuggest)
            return result;
    }
    return result;
}

void HistorySuggestor::setupConnection()
{
    m_historyDb = std::make_unique<sqlite::Database>(m_historyDatabaseFile.toStdString());
    m_statements.insert(std::make_pair(Statement::SearchByWholeInput,
                                       m_historyDb->prepare(R"(SELECT H.VisitID, H.URL, H.Title, H.URLTypedCount, V.VisitCount, V.RecentVisit
                                                            FROM History AS H INNER JOIN
                                                            (SELECT VisitID, MAX(Date) AS RecentVisit, COUNT(Date) AS VisitCount FROM Visits INDEXED BY Visit_ID_Index GROUP BY VisitID) AS V
                                                            ON H.VisitID = V.VisitID
                                                            WHERE H.Title LIKE ? OR H.URL LIKE ?
                                                            ORDER BY V.VisitCount DESC, H.URLTypedCount DESC LIMIT 25)")));
    m_statements.insert(std::make_pair(Statement::SearchBySingleWord,
                                       m_historyDb->prepare(R"(SELECT U.HistoryID, H.URL, H.Title, H.URLTypedCount, V.VisitCount, V.RecentVisit
                                                            FROM URLWords AS U INNER JOIN Words
                                                              ON U.WordID = Words.WordID
                                                            INNER JOIN History AS H
                                                              ON U.HistoryID = H.VisitID
                                                            INNER JOIN (SELECT VisitID, MAX(Date) AS RecentVisit, COUNT(Date) AS VisitCount FROM Visits INDEXED BY Visit_ID_Index GROUP BY VisitID) AS V
                                                              ON H.VisitID = V.VisitID
                                                            WHERE Words.Word LIKE ?
                                                            ORDER BY V.VisitCount DESC, V.RecentVisit DESC, H.URLTypedCount DESC LIMIT 5)")));
}
