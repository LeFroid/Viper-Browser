#include "TextEditorTextFinder.h"

#include <functional>
#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

TextEditorTextFinder::TextEditorTextFinder(QObject *parent) :
    ITextFinder(parent),
    m_editor(nullptr),
    m_lastSearchCursor(),
    m_lastSearchTerm()
{
}

TextEditorTextFinder::~TextEditorTextFinder()
{
}

void TextEditorTextFinder::stopSearching()
{
    if (!m_editor)
        return;

    QTextDocument *doc = m_editor->document();
    QString contents = doc->toPlainText();
    int currentPos = m_editor->textCursor().position();

    Q_EMIT pseudoModifiedDocument();
    doc->setPlainText(contents);

    QTextCursor c = m_editor->textCursor();
    c.setPosition(currentPos);
    m_editor->setTextCursor(c);

    m_lastSearchCursor = QTextCursor();
    m_lastSearchTerm = QString();
    m_searchTerm = QString();
}

void TextEditorTextFinder::findNext()
{
    if (!m_editor)
        return;

    onFindTextResult(true, findText(true));
}

void TextEditorTextFinder::findPrevious()
{
    if (!m_editor)
        return;

    onFindTextResult(false, findText(false));
}

void TextEditorTextFinder::setHighlightAll(bool on)
{
    m_highlightAll = on;

    if (on)
        highlightAllInEditor(m_searchTerm);
    else
        stopSearching();
}

void TextEditorTextFinder::setMatchCase(bool on)
{
    m_matchCase = on;
}

void TextEditorTextFinder::searchTermChanged(const QString &text)
{
    if (text.isEmpty() || m_lastSearchTerm.isEmpty() || !m_searchTerm.startsWith(text))
        m_numOccurrences = 0;

    m_lastSearchTerm = m_searchTerm;
    m_searchTerm = text;
}

void TextEditorTextFinder::setTextEdit(QPlainTextEdit *editor)
{
    m_editor = editor;
}

bool TextEditorTextFinder::findText(bool isFindingNext)
{
    if (!m_editor)
        return false;

    if (m_searchTerm.isEmpty())
    {
        stopSearching();
        return false;
    }

    // Determine which cursor to use
    QTextCursor c { m_lastSearchCursor };

    if (!m_lastSearchTerm.isEmpty() && m_searchTerm.compare(m_lastSearchTerm) != 0)
        c = m_editor->textCursor();

    // Grab the document
    QTextDocument *doc = m_editor->document();

    // Adjust cursor pos based on search direction.
    if (!isFindingNext && c.hasSelection())
    {
        int startSearchPos = c.selectionStart() - 1;
        if (startSearchPos < 0)
            startSearchPos = doc->characterCount() - 1;
        c.clearSelection();
        c.setPosition(startSearchPos, QTextCursor::MoveAnchor);
    }

    QTextDocument::FindFlags flags = isFindingNext ? QTextDocument::FindFlags() : QTextDocument::FindBackward;
    if (m_matchCase)
        flags |= QTextDocument::FindCaseSensitively;

    QTextCursor searchPos = doc->find(m_searchTerm, c, flags);

    // If highlight all option was selected, find all occurrences, and set position to prevPos
    if (m_highlightAll)
        highlightAllInEditor(m_searchTerm);

    // Try to wrap around text if term was not found
    if (searchPos.isNull())
    {
        int charCount = doc->characterCount();

        int nextCursorPos = -1;
        if (isFindingNext && c.position() > 0)
            nextCursorPos = 0;
        else if (!isFindingNext && c.position() < charCount - 1)
            nextCursorPos = charCount - 1;

        if (nextCursorPos >= 0)
        {
            c.setPosition(nextCursorPos);
            searchPos = doc->find(m_searchTerm, c, flags);
        }
    }

    if (!searchPos.isNull())
    {
        m_editor->setTextCursor(searchPos);
        m_editor->ensureCursorVisible();
        return true;
    }

    return false;
}

void TextEditorTextFinder::highlightAllInEditor(const QString &term)
{
    if (!m_editor)
        return;

    QTextDocument *doc = m_editor->document();
    QTextCursor highlightCursor(doc);
    QTextCursor cursorPos = m_editor->textCursor();

    QTextCharFormat highlightFormat = cursorPos.charFormat();
    highlightFormat.setBackground(QBrush(QColor(245, 255, 91)));
    highlightFormat.setForeground(QBrush(QColor(0, 0, 0)));

    while (!highlightCursor.isNull() && !highlightCursor.atEnd())
    {
        highlightCursor = doc->find(term, highlightCursor);
        if (!highlightCursor.isNull())
        {
            Q_EMIT pseudoModifiedDocument();
            highlightCursor.mergeCharFormat(highlightFormat);
        }
    }
}

void TextEditorTextFinder::onFindTextResult(bool isFindingNext, bool isFound)
{
    const bool isEmptySearch = m_searchTerm.isEmpty();
    if (!isFound || isEmptySearch || !m_editor)
    {
        QString resultText = isEmptySearch ? QString() : tr("Phrase not found");
        Q_EMIT showMatchResultText(resultText);
        return;
    }

    if (m_numOccurrences == 0)
    {
        Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
        m_numOccurrences = m_editor->document()->toPlainText().count(m_searchTerm, caseSensitivity);
        m_occurrenceCounter = 0;
    }

    // Update position counter
    int positionModifier = isFindingNext ? 1 : -1;
    m_occurrenceCounter = (m_occurrenceCounter + positionModifier) % (m_numOccurrences + 1);
    if (m_occurrenceCounter == 0)
        m_occurrenceCounter = isFindingNext ? 1 : m_numOccurrences;

    // Send text representation to the UI
    QString resultText = tr("%1 of %2 matches").arg(m_occurrenceCounter).arg(m_numOccurrences);
    Q_EMIT showMatchResultText(resultText);
}
