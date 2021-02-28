#include "BrowserApplication.h"
#include "HTMLHighlighter.h"

HTMLHighlighter::HTMLHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent),
    m_tagRule(),
    m_attributeRule(),
    m_quoteRule(),
    m_doctypeRule(),
    m_commentFormat(),
    m_commentStartExpr("<!--"),
    m_commentEndExpr("-->")
{
    // Set highlighting rules and then base the colors on the overall system theme
    m_tagRule.pattern = QRegularExpression("<(/?)([A-Za-z0-9-]+)");
    m_attributeRule.pattern = QRegularExpression("[a-zA-Z-]+=");
    m_quoteRule.pattern = QRegularExpression("=\"[^\"]*(\"?)");
    m_doctypeRule.pattern = QRegularExpression("<!(\\bdoctype\\b|\\bDOCTYPE\\b)([a-zA-Z0-9= ]*)>");

    if (sBrowserApplication->isDarkTheme())
    {
        m_tagRule.format.setForeground(QBrush(QColor(255, 71, 114)));
        m_attributeRule.format.setForeground(QBrush(QColor(255, 187, 153)));
        m_quoteRule.format.setForeground(QBrush(QColor(91, 219, 255)));
        m_doctypeRule.format.setForeground(QBrush(QColor(224, 224, 224)));
        m_commentFormat.setForeground(QBrush(QColor(133, 255, 147)));
    }
    else
    {
        m_tagRule.format.setForeground(QBrush(QColor(136, 18, 128)));
        m_attributeRule.format.setForeground(QBrush(QColor(153, 69, 0)));
        m_quoteRule.format.setForeground(QBrush(QColor(26, 26, 166)));
        m_doctypeRule.format.setForeground(QBrush(QColor(174, 174, 174)));
        m_commentFormat.setForeground(QBrush(QColor(35, 110, 37)));
    }
}

void HTMLHighlighter::highlightBlock(const QString &text)
{
    QRegularExpression endTag("(/?)>");
    int startIdx = 0, endIdx = 0, captureLen = 0;

    // Search for standard html tags
    auto matchItr = m_tagRule.pattern.globalMatch(text);
    while (matchItr.hasNext())
    {
        auto match = matchItr.next();
        startIdx = match.capturedStart();
        captureLen = match.capturedLength();
        endIdx = startIdx + captureLen;
        setFormat(startIdx, captureLen, m_tagRule.format);

        // Search for attributes within the tag
        int tagEndIdx = text.indexOf(endTag, endIdx);
        if (tagEndIdx == -1)
            setCurrentBlockState(HTMLHighlighter::Tag);

        QString tagSubstr = text.mid(endIdx, tagEndIdx);
        auto attrItr = m_attributeRule.pattern.globalMatch(tagSubstr);
        while (attrItr.hasNext())
        {
            auto attrMatch = attrItr.next();
            setFormat(endIdx + attrMatch.capturedStart(), attrMatch.capturedLength(), m_attributeRule.format);
        }

        // Format at position tagEndIdx
        if (tagEndIdx >= 0)
        {
            int tagEndLen = text.at(tagEndIdx) == '/' ? 2 : 1;
            setFormat(tagEndIdx, tagEndLen, m_tagRule.format);
        }
    }

    // Search for quotes
    matchItr = m_quoteRule.pattern.globalMatch(text);
    while (matchItr.hasNext())
    {
        auto match = matchItr.next();
        setFormat(match.capturedStart() + 1, match.capturedLength() - 1, m_quoteRule.format);
    }

    // Search for doctype
    matchItr = m_doctypeRule.pattern.globalMatch(text);
    if (matchItr.hasNext())
    {
        auto match = matchItr.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_doctypeRule.format);
    }

    int prevBlockState = previousBlockState();
    startIdx = 0;
    endIdx = 0;

    // If previous block state was none, search for first comment and determine the highlighting rule.
    // Otherwise, find a closing tag and then search for highlighting rules
    if (prevBlockState < 1)
        startIdx = text.indexOf(m_commentStartExpr);
    else if (prevBlockState == HTMLHighlighter::Tag)
    {
        // Search for attributes within the tag
        int tagEndIdx = text.indexOf(endTag);
        if (tagEndIdx == -1)
            setCurrentBlockState(HTMLHighlighter::Tag);

        QString tagSubstr = text.left(tagEndIdx + 1);
        auto attrItr = m_attributeRule.pattern.globalMatch(tagSubstr);
        while (attrItr.hasNext())
        {
            auto attrMatch = attrItr.next();
            setFormat(endIdx + attrMatch.capturedStart(), attrMatch.capturedLength(), m_attributeRule.format);
        }

        // Format at position tagEndIdx
        if (tagEndIdx >= 0)
        {
            int tagEndLen = text.at(tagEndIdx) == '/' ? 2 : 1;
            setFormat(tagEndIdx, tagEndLen, m_tagRule.format);
            startIdx = text.indexOf(m_commentStartExpr, tagEndIdx + 1);
        }
        else
            startIdx = -1;
    }

    while (startIdx >= 0)
    {
        auto match = m_commentEndExpr.match(text, startIdx);
        endIdx = match.capturedStart();
        if (endIdx == -1)
        {
            setCurrentBlockState(HTMLHighlighter::Comment);
            captureLen = text.length() - startIdx;
        }
        else
            captureLen = endIdx - startIdx + match.capturedLength();

        setFormat(startIdx, captureLen, m_commentFormat);
        startIdx = text.indexOf(m_commentStartExpr, startIdx + captureLen);
    }
}
