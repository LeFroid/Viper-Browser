#ifndef TEXTEDITORTEXTFINDER_H
#define TEXTEDITORTEXTFINDER_H

#include "ITextFinder.h"

#include <QObject>
#include <QTextCursor>

class QPlainTextEdit;

/**
 * @class TextEditorTextFinder
 * @brief Implements the text finding interface for a text editor
 */
class TextEditorTextFinder final : public ITextFinder
{
    Q_OBJECT

public:
    /// Constructs the text finder with an optional parent
    explicit TextEditorTextFinder(QObject *parent = nullptr);

    /// Destructor
    ~TextEditorTextFinder();

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

    /// Sets the editor that will be used to search for text
    void setTextEdit(QPlainTextEdit *editor);

    /// Searches the text editor for the current search term, returning true if at least one match was found, false if else.
    bool findText(bool isFindingNext);

    /// Highlights all occurrences of the given string in the text editor
    void highlightAllInEditor(const QString &term);

private:
    /**
     * @brief Called when a text search operation is complete
     * @param isFindingNext True if this is for a forward search, false for backwards search
     * @param isFound True if the search term was found, false if else
     */
    void onFindTextResult(bool isFindingNext, bool isFound);

private:
    /// Current text editor
    QPlainTextEdit *m_editor;

    /// Stores the cursor after executing a findNext() or findPrevious() operation.
    /// For some reason the cursor's state is not accurately represened when we attempt
    /// to fetch it again from the editor (it will report no selection, even when there is
    /// a selected text).
    QTextCursor m_lastSearchCursor;

    /// Stores the previous search term (before current one was applied to the text finder)
    QString m_lastSearchTerm;
};

#endif // TEXTEDITORTEXTFINDER_H
