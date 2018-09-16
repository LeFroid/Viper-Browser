#ifndef WEBHITTESTRESULT_H
#define WEBHITTESTRESULT_H

#include <QMap>
#include <QString>
#include <QUrl>
#include <QVariant>

/**
 * @class WebHitTestResult
 * @brief Executes a 'hit test' at a given position in a web view, collecting information about the
 *        elements at that position so they can be used to execute actions in a context menu
 */
class WebHitTestResult
{
public:
    enum MediaType
    {
        MediaTypeNone = 0,
        MediaTypeImage = 1,
        MediaTypeVideo = 2,
        MediaTypeAudio = 3
    };

public:
    /// Constructs the hit test result from the result of executing a hit test on a web page
    explicit WebHitTestResult(const QMap<QString, QVariant> &scriptResultMap);

    /// Returns true if the content in the context is editable, false if else
    bool isContentEditable() const;

    /// Returns the URL of a link if the context is a link
    QUrl linkUrl() const;

    /// Returns the type of media, or MediaTypeNone if the context is not a media element
    MediaType mediaType() const;

    /// Returns the URL of a media element if the media type is not MediaTypeNone
    QUrl mediaUrl() const;

    /// Returns the selected text of the context
    QString selectedText() const;

private:
    bool m_isEditable;

    /// Link url of the context
    QUrl m_linkUrl;

    /// Media url of the context
    QUrl m_mediaUrl;

    /// Media type of the context
    MediaType m_mediaType;

    /// Selected text
    QString m_selectedText;
};

#endif // WEBHITTESTRESULT_H
