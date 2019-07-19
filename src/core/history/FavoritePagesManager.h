#ifndef FAVORITEPAGESMANAGER_H
#define FAVORITEPAGESMANAGER_H

#include <vector>

#include <QDateTime>
#include <QImage>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>

class HistoryManager;
class WebPageThumbnailStore;

/// Stores information about a specific web page, such as its URL, title
/// and a thumbnail
struct WebPageInformation
{
    /// Position of the web page on the favorites web page
    int Position;

    /// Last known title of the page
    QString Title;

    /// URL of the page
    QUrl URL;

    /// A small image of the web page
    QImage Thumbnail;

    /// Returns the thumbnail as a base-64 encoded string
    QString getThumbnailInBase64() const;
};

/// Stores information about an entry that the user removed from the New Tab page
struct NewTabHiddenEntry
{
    /// Url of the hidden entry
    QUrl URL;

    /// Date and time when the entry expires from the list of hidden pages
    QDateTime ExpiresOn;

    /// Default constructor
    NewTabHiddenEntry() : URL(), ExpiresOn() {}

    /// Constructs the entry with a URL and expiry time
    NewTabHiddenEntry(const QUrl &url, QDateTime expiresOn) : URL(url), ExpiresOn(expiresOn) {}
};

/**
 * @class FavoritePagesManager
 * @brief Maintains a list of a user's most frequently visited web pages
 */
class FavoritePagesManager : public QObject
{
    Q_OBJECT

    /// Data file version
    const static QString Version;

public:
    /// Constructs the favorite pages manager, given a pointer to the history manager,
    /// a pointer to the web page thumbnail database, the favorite page data file path,
    /// and an optional parent pointer
    explicit FavoritePagesManager(HistoryManager *historyMgr, WebPageThumbnailStore *thumbnailStore, const QString &dataFile, QObject *parent = nullptr);

    /// Destructor
    ~FavoritePagesManager();

    /// Returns true if the given URL is in the collection of favorite pages, false if it wasn't found
    bool isPresent(const QUrl &url) const;

public Q_SLOTS:
    /// Returns a list of the user's favorite web pages. The QVariants in the list may be converted to \ref WebPageInformation
    QVariantList getFavorites() const;

    /// Adds an item to the list of favorited (pinned) web pages
    void addFavorite(const QUrl &url, const QString &title);

    /// Removes an item from the collection of top/favorite pages
    void removeEntry(const QUrl &url);

protected:
    /// Called on a regular interval to update the list of favorite/most visited pages
    void timerEvent(QTimerEvent *event) override;

private:
    /**
     * @brief Determines if the given URL is in the set of web pages that have been hidden by the user
     * @param url URL in question
     * @return True if the URL is flagged as being hidden, false otherwise
     */
    bool isUrlHidden(const QUrl &url);

    /// Loads the user's favorite pages based on their input
    void loadFavorites();

    /// Loads the user's most frequently visited web pages
    void loadFromHistory();

    /// Saves the lists of favorite pages and excluded pages to disk
    void save();

private:
    /// Unique identifier of the page update timer
    int m_timerId;

    /// Full name, including the path, of the data file in which favorite and hidden page information is stored.
    QString m_dataFileName;

    /// Contains a list of URLs that the user removed from the new tab page
    std::vector<NewTabHiddenEntry> m_excludedPages;

    /// Contains pages that have been explicitly "favorited" or pinned by the user
    /// on the new tab page
    std::vector<WebPageInformation> m_favoritePages;

    /// Contains the user's most frequently visited web pages, ordered from
    /// most to least frequently visited
    std::vector<WebPageInformation> m_mostVisitedPages;

    /// Pointer to the history manager
    HistoryManager *m_historyManager;

    /// Pointer to the web page thumbnail database
    WebPageThumbnailStore *m_thumbnailStore;
};

#endif // FAVORITEPAGESMANAGER_H
