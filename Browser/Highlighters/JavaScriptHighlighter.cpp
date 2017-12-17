#include "JavaScriptHighlighter.h"

JavaScriptHighlighter::JavaScriptHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent),
    m_stdRules(),
    m_nameRule(),
    m_parameterFormat(),
    m_commentFormat(),
    m_singleLineCommentExpr("//.*"),
    m_commentStartExpr("/\\*"),
    m_commentEndExpr("\\*/"),
    m_lastQuoteChar()
{
    // Build keyword rule
    QStringList keywords;
    keywords << "if" << "else" << "of" << "in" << "for" << "while" << "do" << "finally" << "var"
             << "new" <<  "function" << "return" << "void" << "else" << "break" <<  "catch"
             << "instanceof" << "with" << "throw" << "case" << "default" << "try" << "this"
             << "switch"<< "continue" << "typeof" << "delete" << "let" << "yield" << "const"
             << "export" << "super" << "debugger" << "as" << "async" << "await" << "static"
             << "import" << "from" << "as";
    QString keywordStr("\\b(");
    for (const QString &keyword : keywords)
        keywordStr.append(QString("%1|").arg(keyword));
    keywordStr = keywordStr.left(keywordStr.size() - 1);
    keywordStr.append(")\\b");

    HighlightingRule keywordRule;
    keywordRule.pattern = QRegularExpression(keywordStr);
    keywordRule.format.setForeground(QBrush(QColor(136, 18, 128)));
    m_stdRules.push_back(keywordRule);

    // Build name rule (format is only applied to capture group 2 and optionally 4)
    m_nameRule.pattern = QRegularExpression("\\b(var|function|const)\\b ([_\\$a-zA-Z0-9, ]+)((\\(\\s*)([_\\$a-zA-Z0-9, ]+))?");
    m_nameRule.format.setForeground(QBrush(QColor(26, 26, 166)));

    m_parameterFormat.setForeground(QBrush(QColor(136, 18, 128)));

    // Build literals rule
    HighlightingRule literalRule;
    literalRule.pattern = QRegularExpression("\\b(true|false|null|undefined|NaN|Infinity)\\b");
    literalRule.format.setForeground(QBrush(QColor(123, 113, 194)));
    m_stdRules.push_back(literalRule);

    // Built-in objects, properties, methods, etc rule
    keywords.clear();
    keywords << "eval" << "isFinite" << "isNaN" << "parseFloat" << "parseInt" << "decodeURI"
             << "decodeURIComponent" << "encodeURI" << "encodeURIComponent" << "escape"
             << "unescape" << "Object" << "Function" << "Boolean" << "Error" << "EvalError"
             << "InternalError" << "RangeError" << "ReferenceError" << "StopIteration"
             << "SyntaxError" << "TypeError" << "URIError" << "Number" << "Math" << "Date"
             << "String" << "RegExp" << "Array" << "Float32Array" << "Float64Array"
             << "Int16Array" << "Int32Array" << "Int8Array" << "Uint16Array" << "Uint32Array"
             << "Uint8Array" << "Uint8ClampedArray" << "ArrayBuffer" << "DataView" << "JSON"
             << "Intl" << "arguments" << "require" << "module" << "console" << "window"
             << "document" << "Symbol" << "Set" << "Map" << "WeakSet" << "WeakMap"
             << "Proxy" << "Reflect" << "Promise";
    keywordStr = QString("\\b(");
    for (const QString &keyword : keywords)
        keywordStr.append(QString("%1|").arg(keyword));
    keywordStr = keywordStr.left(keywordStr.size() - 1);
    keywordStr.append(")\\b");

    HighlightingRule jsBuiltInRule;
    jsBuiltInRule.pattern = QRegularExpression(keywordStr);
    jsBuiltInRule.format.setForeground(QBrush(QColor(35, 110, 37)));
    m_stdRules.push_back(jsBuiltInRule);

    // Number rule
    HighlightingRule numberRule;
    numberRule.pattern = QRegularExpression("\\b((\\d+)|(0x[\\da-fA-F]+))\\b");
    numberRule.format.setForeground(QBrush(QColor(204, 29, 29)));
    m_stdRules.push_back(numberRule);

    // Quotes rule
    HighlightingRule quotesRule;
    quotesRule.pattern = QRegularExpression("\"([^\"\\\\]*(\\\\.[^\"\\\\]*)*)\"|\\'([^\\'\\\\]*(\\\\.[^\\'\\\\]*)*)\\'|`([^`\\\\]*(\\\\.[^`\\\\]*)*)`");
    quotesRule.format.setForeground(QBrush(QColor(171, 21, 21)));
    m_stdRules.push_back(quotesRule);

    // Comment format
    m_commentFormat.setForeground(QBrush(QColor(153, 69, 0)));
}

void JavaScriptHighlighter::highlightBlock(const QString &text)
{
    // Search standard rules, then name rules, comments and quotes
    for (const HighlightingRule &rule : m_stdRules)
    {
        auto matchItr = rule.pattern.globalMatch(text);
        while (matchItr.hasNext())
        {
            auto match = matchItr.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    auto matchItr = m_nameRule.pattern.globalMatch(text);
    while (matchItr.hasNext())
    {
        auto match = matchItr.next();
        if (match.lastCapturedIndex() >= 2)
            setFormat(match.capturedStart(2), match.capturedLength(2), m_nameRule.format);
        if (match.lastCapturedIndex() > 4)
        {
            // Highlight function parameters, but do not highlight the delimiter ','
            int paramPos = match.capturedStart(5);
            QString substr = text.mid(paramPos, match.capturedLength(5));
            QStringList params = substr.split(QChar(','));
            for(const QString &param : params)
            {
                setFormat(paramPos, param.size(), m_parameterFormat);
                paramPos += param.size() + 1;
            }
        }
    }

    std::vector< std::pair<int, int> > singleLineComments;
    matchItr = m_singleLineCommentExpr.globalMatch(text);
    while (matchItr.hasNext())
    {
        auto match = matchItr.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_commentFormat);
        singleLineComments.push_back({match.capturedStart(), match.capturedStart() + match.capturedLength()});
    }

    // Get previous block state. If state was a comment or quote, try to find ending expression
    int prevBlockState = previousBlockState(), startIdx = 0, endIdx = 0;
    if (prevBlockState == JavaScriptHighlighter::Comment)
    {
        endIdx = text.indexOf(m_commentEndExpr);
        if (endIdx < 0)
        {
            setFormat(0, text.size(), m_commentFormat);
            setCurrentBlockState(JavaScriptHighlighter::Comment);
            return;
        }
        else
        {
            setFormat(0, endIdx + 2, m_commentFormat);
            setCurrentBlockState(JavaScriptHighlighter::None);
            startIdx = endIdx + 3;
        }
    }

    // Search for multi line comments
    startIdx = text.indexOf(m_commentStartExpr, startIdx);
    while (startIdx >= 0)
    {
        // Check if multiline comment was inside a single line comment, if so, ignore
        int newlineIdx = text.lastIndexOf(QChar('\n'), startIdx);
        if (newlineIdx < 0)
            newlineIdx = 0;
        QString lineSubStr = text.mid(newlineIdx, startIdx - newlineIdx);
        auto isCommentedMatch = m_singleLineCommentExpr.globalMatch(lineSubStr);
        if (isCommentedMatch.hasNext())
        {
            startIdx = text.indexOf(m_commentStartExpr, startIdx + 1);
            continue;
        }

        endIdx = text.indexOf(m_commentEndExpr, startIdx);
        if (endIdx < 0)
        {
            setFormat(startIdx, text.size() - startIdx, m_commentFormat);
            setCurrentBlockState(JavaScriptHighlighter::Comment);
            return;
        }
        else
        {
            setFormat(startIdx, 1 + endIdx - startIdx, m_commentFormat);
            startIdx = text.indexOf(m_commentStartExpr, endIdx + 2);
        }
    }
}
