#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BookmarkSuggestor.h"
#include "CommonUtil.h"
#include "FastHash.h"
#include "HistoryManager.h"
#include "Settings.h"

void BookmarkSuggestor::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_bookmarkManager = serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager");
    m_historyManager  = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");

    /*
    if (Settings *settings = serviceLocator.getServiceAs<Settings>("Settings"))
    {
        m_databaseFile = settings->getPathValue(BrowserSetting::BookmarkPath);
    }
    */
}

std::vector<URLSuggestion> BookmarkSuggestor::getSuggestions(const std::atomic_bool &working,
                                                             const QString &searchTerm,
                                                             const QStringList &searchTermParts,
                                                             const FastHashParameters &hashParams)
{
    std::vector<URLSuggestion> result;
    if (!m_bookmarkManager || !m_historyManager)
        return result;

    const int maxToSuggest = 20;
    int numSuggested = 0;

    const QRegularExpression prefixExpr = QRegularExpression(QLatin1String("^WWW\\."));
    const bool inputStartsWithWww = searchTerm.size() >= 3 && searchTerm.startsWith(QLatin1String("WWW"));

    for (const auto &it : *m_bookmarkManager)
    {
        if (!working.load())
            return result;

        if (it->getType() != BookmarkNode::Bookmark)
            continue;

        const QString url = it->getURL().toString();
        MatchType matchType = getMatchType(searchTerm,
                                           searchTermParts,
                                           hashParams,
                                           it->getName().toUpper(),
                                           url.toUpper(),
                                           it->getShortcut().toUpper());

        if (matchType == MatchType::None)
            continue;

        URLSuggestion suggestion { it, m_historyManager->getEntry(it->getURL()), matchType };

        QString suggestionHost = it->getURL().host().toUpper();
        if (!inputStartsWithWww)
            suggestionHost = suggestionHost.replace(prefixExpr, QString());
        suggestion.IsHostMatch = suggestionHost.startsWith(searchTerm);

        result.push_back(suggestion);

        if (++numSuggested >= maxToSuggest)
            return result;
    }

    return result;
}

MatchType BookmarkSuggestor::getMatchType(const QString &searchTerm,
                                          const QStringList &searchTermParts,
                                          const FastHashParameters &hashParams,
                                          const QString &title,
                                          const QString &url,
                                          const QString &shortcut)
{
    if (!shortcut.isEmpty() && searchTerm.startsWith(shortcut))
        return MatchType::Shortcut;

    // Special case for small search terms
    if (searchTerm.size() < 5)
        return getMatchTypeForSmallSearchTerm(searchTerm, title, url);

    if (url.indexOf(searchTerm) >= 0
            && (static_cast<float>(searchTerm.size()) / static_cast<float>(url.size())) >= 0.325f)
            return MatchType::URL;

    const int numWords = searchTermParts.size();
    if (numWords > 1)
    {
        int numMatchingWords = 0;
        for (const QString &word : searchTermParts)
        {
            if (word.size() > 2 && title.contains(word, Qt::CaseSensitive))
                numMatchingWords++;
        }

        if (static_cast<float>(numMatchingWords) / static_cast<float>(numWords) >= 0.5f)
            return MatchType::SearchWords;
    }

    // Try hashing the title and url for a last attempt at matching
    std::wstring haystack = title.toStdWString();
    if (FastHash::isMatch(hashParams.needle, haystack, hashParams.needleHash, hashParams.differenceHash))
        return MatchType::Title;

    haystack = url.toStdWString();
    if (FastHash::isMatch(hashParams.needle, haystack, hashParams.needleHash, hashParams.differenceHash))
        return MatchType::URL;

    return MatchType::None;
}

MatchType BookmarkSuggestor::getMatchTypeForSmallSearchTerm(const QString &searchTerm, const QString &title, const QString &url)
{
    QStringList haystackParts = CommonUtil::tokenizePossibleUrl(title);

    for (const QString &part : haystackParts)
    {
        if (searchTerm.compare(part) == 0)
            return MatchType::Title;
    }

    haystackParts = CommonUtil::tokenizePossibleUrl(url);
    for (const QString &part : qAsConst(haystackParts))
    {
        if (searchTerm.compare(part) == 0)
            return MatchType::URL;
    }

    return MatchType::None;
}
