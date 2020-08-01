#ifndef ITEXTFINDER_H
#define ITEXTFINDER_H

#include <QObject>
#include <QString>

/**
 * @class ITextFinder
 * @brief Defines the interface for any system that wants to use
 *        the \ref FindTextWidget UI in order to search for user
 *        input.
 */
class ITextFinder : public QObject
{
    Q_OBJECT

public:
    /// Constructor - same as a standard QObject
    explicit ITextFinder(QObject *parent = nullptr);

    /// Destructor
    virtual ~ITextFinder() = default;

    /// Tells the text finder to stop searching for text, and to make sure that no prior
    /// text matches are still highlighted.
    virtual void stopSearching() = 0;

public Q_SLOTS:
    /// Finds the next occurrence (if any) of the current search term
    virtual void findNext() = 0;

    /// Finds the previous occurrence (if any) of the current search term
    virtual void findPrevious() = 0;

    /// Sets the flag indicating whether or not all occurrences of the search term should be
    /// highlighted.
    virtual void setHighlightAll(bool on) = 0;

    /// Sets the flag indicating whether or not the search term should be considered case-sensitive.
    virtual void setMatchCase(bool on) = 0;

    /// Handles a search term change event
    virtual void searchTermChanged(const QString &text) = 0;

Q_SIGNALS:
    /// Notifies the \ref FindTextWidget that it should display the given text as the
    /// result of a search operation (ex: "Phrase not found", "2 of 7 matches", "", etc)
    void showMatchResultText(const QString &text);

    /// Emitted when a text document is aesthetically modified by the text finder (where applicable)
    void pseudoModifiedDocument();

protected:
    /// Current search term
    QString m_searchTerm;

    /// Flag indicating whether or not all occurrences of a search term should be highlighted
    bool m_highlightAll;

    /// Flag indicating whether or not the search term should be treated with case sensitivity
    bool m_matchCase;

    /// Number of occurrences for the current search term
    int m_numOccurrences;

    /// Position of the selected occurrence of the current search term,
    /// in range [0, m_numOccurrences) when m_numOccurrences > 0
    int m_occurrenceCounter;
};

#endif // ITEXTFINDER_H
