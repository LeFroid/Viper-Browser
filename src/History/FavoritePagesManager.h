#ifndef FAVORITEPAGESMANAGER_H
#define FAVORITEPAGESMANAGER_H

#include <vector>

#include <QImage>
#include <QObject>
#include <QString>
#include <QUrl>

class HistoryManager;

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
};

/**
 * @class FavoritePagesManager
 * @brief Maintains a list of a user's most frequently visited web pages
 */
class FavoritePagesManager : public QObject
{
    Q_OBJECT

public:
    /// Constructs the favorite pages manager, given a pointer to the history manager and
    /// an optional parent pointer
    explicit FavoritePagesManager(HistoryManager *historyMgr, QObject *parent = nullptr);

    /// Destructor
    ~FavoritePagesManager();

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
};

#endif // FAVORITEPAGESMANAGER_H
