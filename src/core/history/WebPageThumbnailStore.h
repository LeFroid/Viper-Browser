#ifndef WEBPAGETHUMBNAILSTORE_H
#define WEBPAGETHUMBNAILSTORE_H

#include "DatabaseWorker.h"
#include "HistoryManager.h"
#include "ServiceLocator.h"

#include <vector>

#include <QHash>
#include <QImage>
#include <QMimeDatabase>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QUrl>

class BookmarkManager;
class HistoryManager;

/**
 * @class WebPageThumbnailStore
 * @brief A data store that contains thumbnails of web pages that are
 *        either commonly visited, bookmarked or otherwise favorited by
 *        the user.
 */
class WebPageThumbnailStore : public QObject, private DatabaseWorker
{
    friend class BrowserApplication;
    friend class DatabaseFactory;

    Q_OBJECT

public:
    /// Constructs the thumbnail storage manager, given a reference to the service locator, the path to the database file and an optional parent object
    explicit WebPageThumbnailStore(const ViperServiceLocator &serviceLocator, const QString &databaseFile, QObject *parent = nullptr);

    /// Destructor
    ~WebPageThumbnailStore();

    /// Attempts to find a thumbnail associated with the given URL, returning said thumbnail
    /// as a QImage if found, or returning a null pixmap if it could not be found.
    QImage getThumbnail(const QUrl &url);

public Q_SLOTS:
    /// Handles the loadFinished event which is emitted by a \ref WebWidget
    void onPageLoaded(bool ok);

protected:
    /// Called on a regular interval to save thumbnails
    void timerEvent(QTimerEvent *event) override;

    /// Returns true if the thumbnail database contains the table structures needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the table structure if it has not already been created
    void setup() override;

    /// This would load thumbnails from the database into memory, but instead
    /// the thumbnails are loaded as needed and so this does nothing
    void load() override;

private:
    /// Callback registered during a call to save() - handles the query to fetch the most frequently
    /// visited web pages
    void onMostVisitedPagesLoaded(std::vector<WebPageInformation> &&results);

    /// Saves thumbnails of web pages into the database
    void save();

private:
    /// Identifier of the timer that is periodically invoked to call the save() method
    int m_timerId;

    /// Hashmap of web hostnames to their corresponding thumbnails
    QHash<QString, QImage> m_thumbnails;

    /// Pointer to the \ref BookmarkManager
    BookmarkManager *m_bookmarkManager;

    /// Pointer to the \ref HistoryManager
    HistoryManager *m_historyManager;

    /// Database used to check if certain web pages should not be thumbnailed (ex: pictures, movies, etc)
    QMimeDatabase m_mimeDatabase;
};

#endif // WEBPAGETHUMBNAILSTORE_H
