#ifndef URL_H
#define URL_H

#include <QUrl>

/**
 * @class URL
 * @brief Acts as a wrapper for the \ref QUrl class, extending its functionality
 */
class URL : public QUrl
{
public:
    /// Default constructor
    URL();

    /// Constructs a copy of the given URL
    URL(const QUrl &other);

    /// Constructs a copy of the given URL
    URL(const URL &other);

    /// Constructs a URL from the given string a parsing mode
    URL(const QString &url, ParsingMode parsingMode = URL::TolerantMode);

    /// Returns the second-level domain of the URL (ex: websiteA.com; websiteB.co.uk)
    QString getSecondLevelDomain() const;
};

#endif // URL_H
