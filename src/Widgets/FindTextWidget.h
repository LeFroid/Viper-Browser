#ifndef FINDTEXTWIDGET_H
#define FINDTEXTWIDGET_H

#include <vector>
#include <QWidget>

namespace Ui {
class FindTextWidget;
}

class QLineEdit;
class QPlainTextEdit;
class WebView;

/**
 * @class FindTextWidget
 * @brief Used to search for any given text in either a web page or a text document
 */
class FindTextWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the text finder widget with the given parent
    explicit FindTextWidget(QWidget *parent = 0);

    /// FindTextWidget destructor
    ~FindTextWidget();

    /// Sets the pointer to a plain text editor, in which terms will be searched.
    void setTextEdit(QPlainTextEdit *editor);

    /// Sets the pointer to the active web view
    void setWebView(WebView *view);

    /// Returns a pointer to the line edit widget (used to acquire focus)
    QLineEdit *getLineEdit();

signals:
    /// Emitted when a text document is aesthetically modified by the text finder
    void pseudoModifiedDocument();

public slots:
    /// Resets any highlighting operations that were performed on the text document. Not applicable to web page searches
    void resetEditorChanges();

private slots:
    /// Search for the next occurrence of the string in the line edit
    void onFindNext();

    /// Search for the last occurrence of the string in the line edit
    void onFindPrev();

    /// Toggles the highlighting of all or one occurrence of the search term
    void onToggleHighlightAll(bool checked);

    /// Toggles search term case sensitivity
    void onToggleMatchCase(bool checked);

    /// Searches within the text editor for the current search term, returning true if at least one match was found, false if else.
    bool findInEditor(bool nextMatch = true);

    /// Highlights all occurrences of the given string in a text editor
    void highlightAllInEditor(const QString &term);

private:
    /// Sets the label that displays the number of matches found for the given search term.
    /// If searchForNext = true, will increment the position counter. Otherwise, the counter will decrement.
    void setMatchCountLabel(bool searchForNext);

    /// Callback used due to multi-process architecture of webengine
    void setMatchCountLabelCallback(bool searchForNext, const QString &documentText);

protected:
    /// Paints the widget onto its parent. This method is overrided in order to draw a border around the widget
    virtual void paintEvent(QPaintEvent *event) override;

private:
    /// Find text UI
    Ui::FindTextWidget *ui;

    /// Active text editor. If this is set, the web view will not be used
    QPlainTextEdit *m_editor;

    /// Active web view. If this is set, the text document will not be used
    WebView *m_view;

    /// Current search term
    QString m_searchTerm;

    /// Number of occurrences for current search term
    int m_numOccurrences;

    /// Counter for which of the search term results the user is currently viewing
    int m_occurrenceCounter;

    /// Flag for highlighting all occurrences of a search term
    bool m_highlightAll;

    /// Flag for case sensitivity
    bool m_matchCase;
};

#endif // FINDTEXTWIDGET_H
