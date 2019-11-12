#ifndef IURLSUGGESTOR_H
#define IURLSUGGESTOR_H

#include <atomic>
#include <string>
#include <vector>

#include <QtGlobal>
#include <QString>
#include <QStringList>

#include "ServiceLocator.h"

struct URLSuggestion;

struct FastHashParameters
{
    /// Needle that is hashed to check for its presence in a haystack
    std::wstring needle;

    /// Used for string hash comparisons in the FastHash implementation
    quint64 differenceHash;

    /// Precomputed hash of the needle
    quint64 needleHash;
};

/**
 * @class IURLSuggestor
 * @brief Interface for any classes that feed
 *        suggestions to the \ref URLSuggestionWorker for
 *        URL entries that are based on some user input.
 */
class IURLSuggestor
{
public:
    /// Default constructor
    IURLSuggestor() = default;

    /// Destructor
    virtual ~IURLSuggestor() = default;

    /// Used for generic dependency injection
    virtual void setServiceLocator(const ViperServiceLocator &serviceLocator) = 0;

    /**
     * @brief getSuggestions Finds recommendations for the user based on their input, as well as the suggestor's data source
     * @param working Flag indicating whether or not the calling suggestion worker is still active. When set to false,
     *        the URL suggestor implementation should return immediately
     * @param searchTerm User input string
     * @param searchTermParts The user input, broken into tokens based on a number of criteria (spaces, letter-number boundaries, etc.)
     * @param hashParams Pre-computed hash inputs, used to perform raw string comparisons against the potential url suggestions
     * @return A vector of \ref URLSuggestion objects, which can be empty if no suggestions are found
     */
    virtual std::vector<URLSuggestion> getSuggestions(const std::atomic_bool &working,
                                                      const QString &searchTerm,
                                                      const QStringList &searchTermParts,
                                                      const FastHashParameters &hashParams) = 0;
};

#endif // IURLSUGGESTOR_H
