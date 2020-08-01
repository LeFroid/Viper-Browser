#ifndef COMMONUTIL_H
#define COMMONUTIL_H

#include <string>
#include <functional>

#include <QIcon>
#include <QRegularExpression>
#include <QString>
#include <QtGlobal>
#include <QUrl>

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))

/// Allows use of QString in standard hashmaps and the like
namespace std
{
    template<> struct hash<QString>
    {
        std::size_t operator()(const QString &s) const
        {
            return static_cast<std::size_t>(qHash(s));
        }
    };
}

#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    using QStringSplitFlag = Qt::SplitBehaviorFlags;
#else
    using QStringSplitFlag = QString::SplitBehavior;
#endif

/// Collection of common utility functions
namespace CommonUtil
{
    /// Converts the given number of bytes into a user-readable string (ex: "10 MB", "1.2 GB" etc)
    QString bytesToUserFriendlyStr(quint64 amount);

    /// Converts the base64-encoded byte array into a QIcon
    QIcon iconFromBase64(QByteArray data);

    /// Returns the base64 encoding of the given icon
    QByteArray iconToBase64(QIcon icon);

    /// Computes and returns base^exp
    quint64 quPow(quint64 base, quint64 exp);

    /// Converts the match pattern into a QRegularExpression that can be applied to URLs
    /// See https://developer.chrome.com/extensions/match_patterns for match pattern specification
    QRegularExpression getRegExpForMatchPattern(const QString &str);

    /// Returns true if the two URLs are the same, false otherwise.
    bool doUrlsMatch(const QUrl &a, const QUrl &b, bool ignoreScheme = false);

    /// Tokenizes the given input string into a list of words
    /// The string may or may not be a URL - depending on the caller - but
    /// URL tokenization rules are applied regardless
    QStringList tokenizePossibleUrl(QString str);
}

#endif // COMMONUTIL_H
