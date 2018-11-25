#ifndef WEBHISTORY_H
#define WEBHISTORY_H

#include <QByteArray>
#include <QDateTime>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QUrl>

#include <deque>
#include <vector>

class QAction;
class WebPage;

/**
 * @struct WebHistoryEntry
 * @brief Represents one history entry associated with a \ref WebPage
 */
struct WebHistoryEntry
{
    friend class WebHistory;

    /// Favicon associated with the history item's URL
    QIcon icon;

    /// Title of the page
    QString title;

    /// URL of the page
    QUrl url;

    /// Time of visit
    QDateTime visitTime;

    /// Default constructor
    WebHistoryEntry() = default;

    /// Constructs the WebHistoryEntry with a given icon, page title and url
    WebHistoryEntry(const QIcon &icon, const QString &title, const QUrl &url) :
        icon(icon),
        title(title),
        url(url),
        visitTime(QDateTime::currentDateTime()),
        page(nullptr)
    {
    }

protected:
    /// Page associated with the history entry
    WebPage *page;
};

/**
 * @class WebHistory
 * @brief Represents the history of a \ref WebPage, and exposes methods
 *        to travel back or forward in the page's history
 */
class WebHistory : public QObject
{
    Q_OBJECT

    const static QString SerializationVersion;

public:
    /// Constructs the WebHistory with a given parent
    explicit WebHistory(WebPage *parent);

    /// Destructor
    ~WebHistory();

    /**
     * @brief Returns a list of the web page's previously visited history entries
     * @param maxEntries The maximum number of entries to be included in the list, if maxEntries is >= 0
     * @return List of history entries visited before the current page's entry
     */
    std::vector<WebHistoryEntry> getBackEntries(int maxEntries = -1) const;

    /**
     * @brief Returns a list of the web page's forward-facing history entries
     * @param maxEntries The maximum number of entries to be included in the list, if maxEntries is >= 0
     * @return List of history entries visited after the current page's entry
     */
    std::vector<WebHistoryEntry> getForwardEntries(int maxEntries = -1) const;

    /// Returns true if the page can go back by one entry in its history, false if else
    bool canGoBack() const;

    /// Returns true if the page can go forward by one entry in its history, false if else
    bool canGoForward() const;

    /// Loads the history from a byte array
    void load(QByteArray &data);

    /// Saves the web page history, returning it as a byte array
    QByteArray save() const;

signals:
    /// Emitted when any of the history associated with a page has changed
    void historyChanged();

public slots:
    /// Moves the page back to the page associated with the previous history entry
    void goBack();

    /// Moves the page forward to the page associated with the next history entry
    void goForward();

    /// Goes to and loads the specified history entry, so long as the entry is valid
    void goToEntry(const WebHistoryEntry &entry);

    /// Reloads the current page
    void reload();

private slots:
    /// Handles a page load event
    void onPageLoaded(bool ok);

    /// Handles a URL change event
    void onUrlChanged(const QUrl &url);

private:
    /// Pointer to the web page
    WebPage *m_page;

    /// Pointer to the web page's go back action
    QAction *m_backAction;

    /// Pointer to the web page's go forward action
    QAction *m_forwardAction;

    /// Double-ended queue of history entries.
    /// At the front of queue are the oldest entries, the end of the queue contains the newest.
    std::deque<WebHistoryEntry> m_entries;

    /// Position of the entry associated with the page's current URL
    int m_currentPos;

    /// Position of an entry being loaded (eg going back to the item at this index, going forward to this index)
    int m_targetPos;
};

#endif // WEBHISTORY_H
