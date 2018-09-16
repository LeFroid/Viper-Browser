#ifndef HTMLHIGHLIGHTER_H
#define HTMLHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QVector>

/**
 * @class HTMLHighlighter
 * @brief Acts as a syntax highlighter when viewing HTML source code
 */
class HTMLHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    /// HTMLHighlighter constructor
    HTMLHighlighter(QTextDocument *parent = nullptr);

protected:
    /// Highlights the block of text according to the HTML syntax rules
    void highlightBlock(const QString &text) override;

private:
    /// States of the highlighter as it processes a block
    enum States
    {

        None    = 0, /// When no patterns match current text
        Tag     = 1, /// Highlighting contents within an HTML tag
        Comment = 2  /// Commented out section of code <!-- like so -->
    };

    /// Highlighting rule for certain patterns of text
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    /// HTML tag highlighting rule
    HighlightingRule m_tagRule;

    /// HTML attribute highlighting rule
    HighlightingRule m_attributeRule;

    /// HTML quote highlighting rule
    HighlightingRule m_quoteRule;

    /// HTML doctype highlighting rule
    HighlightingRule m_doctypeRule;

    /// Comment text format
    QTextCharFormat m_commentFormat;

    /// Comment start expression
    QRegularExpression m_commentStartExpr;

    /// Comment end expression
    QRegularExpression m_commentEndExpr;
};

#endif // HTMLHIGHLIGHTER_H
