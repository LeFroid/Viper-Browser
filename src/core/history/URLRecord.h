#ifndef URLRECORD_H
#define URLRECORD_H

#include "SQLiteWrapper.h"
#include "../database/bindings/QtSQLite.h"

#include <utility>
#include <vector>

#include <QDateTime>
#include <QString>
#include <QUrl>

using VisitEntry = QDateTime;

/**
 * @struct HistoryEntry
 * @brief Contains data about a specific web URL visited by the user
 */
struct HistoryEntry final : public sqlite::Row
{
    /// URL of the item
    QUrl URL;

    /// Title of the web page
    QString Title;

    /// Unique visit ID of the history entry
    int VisitID;

    /// The last time the user visited this entry
    VisitEntry LastVisit;

    /// The number of visits to this entry
    int NumVisits;

    /// The number of times the URL associated with this entry was typed by the user in the URL bar
    int URLTypedCount;

    /// Default constructor
    HistoryEntry() : URL(), Title(), VisitID(0), LastVisit(), NumVisits(0), URLTypedCount(0) {}

    /// Copy constructor
    HistoryEntry(const HistoryEntry &other) :
        URL(other.URL),
        Title(other.Title),
        VisitID(other.VisitID),
        LastVisit(other.LastVisit),
        NumVisits(other.NumVisits),
        URLTypedCount(other.URLTypedCount)
    {
    }

    /// Move constructor
    HistoryEntry(HistoryEntry &&other) noexcept :
        URL(other.URL),
        Title(other.Title),
        VisitID(other.VisitID),
        LastVisit(other.LastVisit),
        NumVisits(other.NumVisits),
        URLTypedCount(other.URLTypedCount)
    {
    }

    virtual ~HistoryEntry(){}

    /// Copy assignment operator
    HistoryEntry &operator =(const HistoryEntry &other)
    {
        if (this != &other)
        {
            URL = other.URL;
            Title = other.Title;
            VisitID = other.VisitID;
            LastVisit = other.LastVisit;
            NumVisits = other.NumVisits;
            URLTypedCount = other.URLTypedCount;
        }

        return *this;
    }

    /// Move assignment operator
    HistoryEntry &operator =(HistoryEntry &&other) noexcept
    {
        if (this != &other)
        {
            URL = other.URL;
            Title = other.Title;
            VisitID = other.VisitID;
            LastVisit = other.LastVisit;
            NumVisits = other.NumVisits;
            URLTypedCount = other.URLTypedCount;
        }

        return *this;
    }

    /// Returns true if the two HistoryEntry objects are the same, false if else
    bool operator ==(const HistoryEntry &other) const
    {
        return (VisitID == other.VisitID || URL.toString().compare(other.URL.toString(), Qt::CaseSensitive) == 0);
    }

    void marshal(sqlite::PreparedStatement &stmt) const override
    {
        stmt << VisitID
             << URL
             << Title
             << URLTypedCount;
    }

    void unmarshal(sqlite::PreparedStatement &stmt) override
    {
        stmt >> VisitID
             >> URL
             >> Title
             >> URLTypedCount
             >> NumVisits
             >> LastVisit;
    }
};

/**
 * @class URLRecord
 * @brief Contains a full record of a URL in the history database,
 *        including its \ref HistoryEntry and all \ref VisitEntry records.
 */
class URLRecord
{
    friend class HistoryManager;

public:
    /// Constructs the URL record given the associated history entry and a list of visits
    explicit URLRecord(HistoryEntry &&entry, std::vector<VisitEntry> &&visits) noexcept;

    /// Constructs the URL record given a history entry
    explicit URLRecord(HistoryEntry &&entry) noexcept;

    /// Returns the last time that the user visited this entry
    const VisitEntry &getLastVisit() const;

    /// Returns the number of visits made to this entry
    int getNumVisits() const;

    /// Returns the number of times this record was typed by the user in the URL bar
    int getUrlTypedCount() const;

    /// Returns the URL associated with the record
    const QUrl &getUrl() const;

    /// Returns the title of the web page
    const QString &getTitle() const;

    /// Returns the unique visit identifier
    int getVisitId() const;

    /// Returns a reference to the visits associated with this record
    const std::vector<VisitEntry> &getVisits() const;

protected:
    /// Adds a visit to the record
    void addVisit(VisitEntry entry);

    /// The history entry, containg the URL, title, last visit and visit count
    HistoryEntry m_historyEntry;

    /// List of every visit stored in the database
    std::vector<VisitEntry> m_visits;
};

#endif // URLRECORD_H
