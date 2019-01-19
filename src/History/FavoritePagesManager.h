#ifndef FAVORITEPAGESMANAGER_H
#define FAVORITEPAGESMANAGER_H

#include <vector>

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
    /// Last known title of the page
    QString Title;

    /// URL of the page
    QUrl URL;

    /// A small image of the web page
    QImage Thumbnail;

    /// Returns the thumbnail as a base-64 encoded string
    QString getThumbnailInBase64() const;
};

/**
 * @class FavoritePagesManager
 * @brief Maintains a list of a user's most frequently visited web pages
 */
class FavoritePagesManager : public QObject
{
    Q_OBJECT

public:
    /// Constructs the favorite pages manager, given a pointer to the history manager,
    /// a pointer to the web page thumbnail database, an optional parent pointer
    explicit FavoritePagesManager(HistoryManager *historyMgr, WebPageThumbnailStore *thumbnailStore, QObject *parent = nullptr);

    /// Destructor
    ~FavoritePagesManager();

public slots:
    /// Returns a list of the user's favorite web pages. The QVariants in the list may be converted to \ref WebPageInformation
    QVariantList getFavorites() const;

protected:
    /// Called on a regular interval to update the list of favorite/most visited pages
    void timerEvent(QTimerEvent *event) override;

private:
    /// Loads the user's favorite pages based on browsing history and their own input.
    void loadFavorites();

private:
    /// Unique identifier of the page update timer
    int m_timerId;

    /// Contains the user's most frequently visited web pages, ordered from
    /// most to least frequently visited
    std::vector<WebPageInformation> m_mostVisitedPages;

    /// Pointer to the history manager
    HistoryManager *m_historyManager;

    /// Pointer to the web page thumbnail database
    WebPageThumbnailStore *m_thumbnailStore;
};

#endif // FAVORITEPAGESMANAGER_H
