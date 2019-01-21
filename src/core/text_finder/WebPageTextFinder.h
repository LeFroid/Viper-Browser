#ifndef WEBPAGETEXTFINDER_H
#define WEBPAGETEXTFINDER_H

#include "ITextFinder.h"
#include <QObject>

class WebPage;

/**
 * @class WebPageTextFinder
 * @brief Implements the text finding interface for a web page
 */
class WebPageTextFinder final : public ITextFinder
{
    Q_OBJECT

public:
    /// Constructs the web page text finder with an optional parent
    explicit WebPageTextFinder(QObject *parent = nullptr);

    /// Destructor
    ~WebPageTextFinder();

    /// Tells the text finder to stop searching for text, and to make sure that no prior
    /// text matches are still highlighted.
    void stopSearching() override;

    /// Finds the next occurrence (if any) of the current search term
    void findNext() override;

    /// Finds the previous occurrence (if any) of the current search term
    void findPrevious() override;

    /// Sets the flag indicating whether or not all occurrences of the search term should be
    /// highlighted.
    void setHighlightAll(bool on) override;

    /// Sets the flag indicating whether or not the search term should be considered case-sensitive.
    void setMatchCase(bool on) override;

    /// Handles a search term change event
    void searchTermChanged(const QString &text) override;

    /// Sets the web page that will be used to search for text
    void setWebPage(WebPage *page);

private:
    /**
     * @brief Called when a text search operation is complete
     * @param isFindingNext True if this is for a forward search, false for backwards search
     * @param isFound True if the search term was found, false if else
     */
    void onFindTextResult(bool isFindingNext, bool isFound);

    /**
     * @brief Called when the text contained in the current web page have been extracted
     * @param isFindingNext True if this is for a forward search, false for backwards search
     * @param text Text contents of the web page
     */
    void onRetrieveWebPageText(bool isFindingNext, const QString &text);

    /// Updates the relative position of the search result and emits a textual representation to the
    /// \ref FindTextWidget (ex: 5 matches found, position is at the second match, emit "2 of 5 matches")
    void updatePositionTracker(bool isFindingNext);

private:
    /// Points to the web page in which the text is being searched
    WebPage *m_page;

    /// Stores the previous search term (before current one was applied to the text finder)
    QString m_lastSearchTerm;
};

#endif // WEBPAGETEXTFINDER_H
