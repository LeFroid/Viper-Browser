#ifndef WEBSTATE_H
#define WEBSTATE_H

#include <QByteArray>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QUrl>

class BrowserTabWidget;
class WebWidget;

/**
 * @struct WebState
 * @brief Contains the information about a \ref WebWidget that is needed to persist
 *        the state of a web page throughout multiple browsing sessions, as well as
 *        hibernation of a web page.
 */
struct WebState
{
    /// The index of the page in its parent \ref BrowserTabWidget
    int index;

    /// True if the page is pinned in its \ref BrowserTabWidget, false if else
    bool isPinned;

    /// The icon associated with the page at its URL
    QIcon icon;

    /// The URL of the icon
    QUrl iconUrl;

    /// The title of the page its URL
    QString title;

    /// The page's current URL
    QUrl url;

    /// Serialized history of the page
    QByteArray pageHistory;

    /// Default constructor
    WebState() = default;

    /// Constructs the WebState, given a pointer to the WebWidget and its parent BrowserTabWidget
    WebState(WebWidget *webWidget, BrowserTabWidget *tabWidget);

    /// Copy constructor
    WebState(const WebState &other) = default;

    /// Copy assignment operator
    WebState &operator=(const WebState &other) = default;

    /// Move constructor
    WebState(WebState &&other) noexcept;

    /// Move assignment operator
    WebState &operator=(WebState &&other) noexcept;

    /// Destructor
    ~WebState() = default;

    /// Serializes the WebState into a byte array
    QByteArray serialize() const;

    /// Deserializes the WebState from the given byte array
    void deserialize(QByteArray &data);
};

#endif // WEBSTATE_H
