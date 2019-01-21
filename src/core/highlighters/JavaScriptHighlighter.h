#ifndef JAVASCRIPTHIGHLIGHTER_H
#define JAVASCRIPTHIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <vector>

/**
 * @class JavaScriptHighlighter
 * @brief Acts as a syntax highlighter when viewing and/or modifying JavaScript code
 */
class JavaScriptHighlighter : public QSyntaxHighlighter
{
     Q_OBJECT
public:
    /// JavaScript highlighter constructor
    JavaScriptHighlighter(QTextDocument *parent = nullptr);

protected:
    /// Highlights the block of text according to the JavaScript syntax rules
    void highlightBlock(const QString &text) override;

private:
    /// States of the highlighter as it processes a block
    enum States
    {

        None    = 0,  /// When no patterns match current text
        Comment = 1  /// Commented out section of code
    };

    /// Highlighting rule for certain patterns of text
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    /// Vector of standard highlighting rules
    std::vector<HighlightingRule> m_stdRules;

    /// Variable and function name highlighting rule
    HighlightingRule m_nameRule;

    /// Function parameters format
    QTextCharFormat m_parameterFormat;

    /// Comment format
    QTextCharFormat m_commentFormat;

    /// Single-line comment expression
    QRegularExpression m_singleLineCommentExpr;

    /// Multi-line comment start expression
    QRegularExpression m_commentStartExpr;

    /// Multi-line comment end expression
    QRegularExpression m_commentEndExpr;

    /// Character that was used to start a quote in the last block, if the block did not end the same quote
    QChar m_lastQuoteChar;
};

#endif // JAVASCRIPTHIGHLIGHTER_H
