#ifndef WEBPAGETHUMBNAILSTORE_H
#define WEBPAGETHUMBNAILSTORE_H

#include "DatabaseWorker.h"

#include <vector>

#include <QHash>
#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QString>
#include <QUrl>

class WebView;

/**
 * @class WebPageThumbnailStore
 * @brief A data store that contains thumbnails of web pages that are
 *        either commonly visited, bookmarked or otherwise favorited by
 *        the user.
 */
class WebPageThumbnailStore : public QObject, private DatabaseWorker
{
    friend class DatabaseFactory;

    Q_OBJECT

public:
    /// Constructs the thumbnail storage manager, given the path to the database file and an optional parent object
    explicit WebPageThumbnailStore(const QString &databaseFile, QObject *parent = nullptr);

    /// Destructor
    ~WebPageThumbnailStore();

    /// Attempts to find a thumbnail associated with the given URL, returning said thumbnail
    /// as a QImage if found, or returning a null pixmap if it could not be found.
    QImage getThumbnail(const QUrl &url);

public slots:
    /// Handles the loadFinished event which is emitted by a \ref WebWidget
    void onPageLoaded(bool ok);

protected:
    /// Returns true if the thumbnail database contains the table structures needed for it to function properly,
    /// false if else.
    bool hasProperStructure() override;

    /// Creates the table structure if it has not already been created
    void setup() override;

    /// This would load thumbnails from the database into memory, but instead
    /// the thumbnails are loaded as needed and so this does nothing
    void load() override;

    /// Saves thumbnails of web pages into the database
    void save() override;

private:
    /// Hashmap of web hostnames to their corresponding thumbnails
    QHash<QString, QImage> m_thumbnails;
};

#endif // WEBPAGETHUMBNAILSTORE_H
