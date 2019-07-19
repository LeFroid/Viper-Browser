#ifndef WEBHISTORY_H
#define WEBHISTORY_H

#include "ServiceLocator.h"

#include <QByteArray>
#include <QDateTime>
#include <QIcon>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>

#include <vector>

class QAction;
class FaviconManager;
class WebPage;

using WebHistoryEntryImpl = QWebEngineHistoryItem;
using WebHistoryImpl = QWebEngineHistory;

/**
 * @struct WebHistoryEntry
 * @brief Represents one history entry associated with a \ref WebPage
 */
struct WebHistoryEntry
{
    /// Favicon associated with the history item's URL
    QIcon icon;

    /// Title of the page
    QString title;

    /// URL of the page
    QUrl url;

    /// Time of visit
    QDateTime visitTime;

    /// Implementation of the web history entry
    WebHistoryEntryImpl impl;

    /// Constructs the history entry given a reference to the history entry implementation class
    WebHistoryEntry(FaviconManager *faviconManager, const WebHistoryEntryImpl &impl);
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
    explicit WebHistory(const ViperServiceLocator &serviceLocator, WebPage *parent);

    /// Destructor
    ~WebHistory();

    /**
     * @brief Returns a list of the web page's previously visited history entries
     * @param maxEntries The maximum number of entries to be included in the list
     * @return List of history entries visited before the current page's entry
     */
    std::vector<WebHistoryEntry> getBackEntries(int maxEntries = 10) const;

    /**
     * @brief Returns a list of the web page's forward-facing history entries
     * @param maxEntries The maximum number of entries to be included in the list
     * @return List of history entries visited after the current page's entry
     */
    std::vector<WebHistoryEntry> getForwardEntries(int maxEntries = 10) const;

    /// Returns true if the page can go back by one entry in its history, false if else
    bool canGoBack() const;

    /// Returns true if the page can go forward by one entry in its history, false if else
    bool canGoForward() const;

    /// Loads the history from a byte array
    void load(QByteArray &data);

    /// Saves the web page history, returning it as a byte array
    QByteArray save() const;

Q_SIGNALS:
    /// Emitted when any of the history associated with a page has changed
    void historyChanged();

public Q_SLOTS:
    /// Moves the page back to the page associated with the previous history entry
    void goBack();

    /// Moves the page forward to the page associated with the next history entry
    void goForward();

    /// Goes to and loads the specified history entry, so long as the entry is valid
    void goToEntry(const WebHistoryEntry &entry);

private:
    /// Points to the favicon manager
    FaviconManager *m_faviconManager;

    /// Pointer to the web page
    WebPage *m_page;

    /// Implementation of the WebHistory functionality
    WebHistoryImpl *m_impl;
};

#endif // WEBHISTORY_H
