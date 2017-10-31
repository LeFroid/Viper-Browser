#ifndef FINDTEXTWIDGET_H
#define FINDTEXTWIDGET_H

#include <QWidget>

namespace Ui {
class FindTextWidget;
}

class QLineEdit;
class WebView;

/**
 * @class FindTextWidget
 * @brief Used to search for any given text in the active web page
 */
class FindTextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FindTextWidget(QWidget *parent = 0);
    ~FindTextWidget();

    /// Sets the pointer to the active web view
    void setWebView(WebView *view);

    /// Returns a pointer to the line edit widget (used to acquire focus)
    QLineEdit *getLineEdit();

private slots:
    /// Search for the next occurrence of the string in the line edit
    void onFindNext();

    /// Search for the last occurrence of the string in the line edit
    void onFindPrev();

    /// Toggles the highlighting of all or one occurrence of the search term
    void onToggleHighlightAll(bool checked);

    /// Toggles search term case sensitivity
    void onToggleMatchCase(bool checked);

private:
    /// Sets the label that displays the number of matches found for the given search term.
    /// If searchForNext = true, will increment the position counter. Otherwise, the counter will decrement.
    void setMatchCountLabel(bool searchForNext);

private:
    /// Find text UI
    Ui::FindTextWidget *ui;

    /// Active web view
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
