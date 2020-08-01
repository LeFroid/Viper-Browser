#include "CommonUtil.h"
#include "HistoryStore.h"

#include <QDateTime>
#include <QUrl>
#include <QDebug>

HistoryStore::HistoryStore(const QString &databaseFile) :
    DatabaseWorker(databaseFile),
    m_lastVisitID(0),
    m_statements()
{
    m_database.execute("PRAGMA foreign_keys=\"0\"");
}

HistoryStore::~HistoryStore()
{
    m_statements.clear();
}

void HistoryStore::clearAllHistory()
{
    if (!exec(QLatin1String("DELETE FROM URLWords")))
        qWarning() << "In HistoryStore::clearAllHistory - Unable to clear History Word Mapping table.";

    if (!exec(QLatin1String("DELETE FROM Words")))
        qWarning() << "In HistoryStore::clearAllHistory - Unable to clear History Word table.";

    if (!exec(QLatin1String("DELETE FROM History")))
        qWarning() << "In HistoryStore::clearAllHistory - Unable to clear History table.";

    if (!exec(QLatin1String("DELETE FROM Visits")))
        qWarning() << "In HistoryStore::clearAllHistory - Unable to clear Visits table.";
}

void HistoryStore::clearHistoryFrom(const QDateTime &start)
{
    auto stmt = m_database.prepare(R"(DELETE FROM Visits WHERE Date >= ?)");
    stmt << start;
    if (!stmt.execute())
        qWarning() << "In HistoryStore::clearHistoryFrom - Unable to clear history.";

    if (!m_database.execute("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)"))
        qWarning() << "In HistoryStore::clearHistoryFrom - Unable to clear history.";
}

void HistoryStore::clearHistoryInRange(std::pair<QDateTime, QDateTime> range)
{
    auto stmt = m_database.prepare(R"(DELETE FROM Visits WHERE Date >= ? AND Date <= ?)");
    stmt << range.first
         << range.second;
    if (!stmt.execute())
        qWarning() << "In HistoryStore::clearHistoryInRange - Unable to clear history.";

    if (!m_database.execute("DELETE FROM History WHERE VisitID NOT IN (SELECT DISTINCT VisitID FROM Visits)"))
        qWarning() << "In HistoryStore::clearHistoryInRange - Unable to clear history. ";
}

bool HistoryStore::contains(const QUrl &url) const
{
    auto stmt = m_database.prepare(R"(SELECT VisitID FROM History WHERE URL = ?)");
    stmt << url;
    return stmt.next();
}

HistoryEntry HistoryStore::getEntry(const QUrl &url)
{
    HistoryEntry result;
    result.URL = url;
    result.VisitID = -1;

    sqlite::PreparedStatement &stmt = m_statements.at(Statement::GetHistoryRecord);
    stmt.reset();
    stmt << url;
    if (stmt.next())
        stmt >> result;

    return result;
}

std::vector<VisitEntry> HistoryStore::getVisits(const HistoryEntry &record)
{
    std::vector<VisitEntry> result;

    auto stmt = m_database.prepare(R"(SELECT Date FROM Visits WHERE VisitID = ? ORDER BY Date ASC)");
    stmt << record.VisitID;
    while (stmt.next())
    {
        QDateTime currentDate;
        stmt >> currentDate;
        result.push_back(currentDate);
    }

    return result;
}

std::deque<HistoryEntry> HistoryStore::getRecentItems()
{
    std::deque<HistoryEntry> result;

    auto stmt = m_database.prepare(R"(SELECT Visits.VisitID, History.URL, History.Title,
     History.URLTypedCount, 1, Visits.Date FROM Visits
     INNER JOIN History ON Visits.VisitID = History.VisitID
     ORDER BY Visits.Date DESC LIMIT 15;)");

    while (stmt.next())
    {
        HistoryEntry entry;
        stmt >> entry;
        result.push_back(std::move(entry));
    }

    return result;
}

std::vector<URLRecord> HistoryStore::getHistoryFrom(const QDateTime &startDate) const
{
    return getHistoryBetween(startDate, QDateTime::currentDateTime());
}

std::vector<URLRecord> HistoryStore::getHistoryBetween(const QDateTime &startDate, const QDateTime &endDate) const
{
    std::vector<URLRecord> result;

    if (!startDate.isValid() || !endDate.isValid())
        return result;

    auto queryVisitIds = m_database.prepare(R"(SELECT DISTINCT VisitID FROM Visits WHERE Date >= ? AND Date <= ?
                                            ORDER BY Date ASC)");
    auto queryHistoryItem = m_database.prepare(R"(SELECT URL, Title, URLTypedCount FROM History WHERE VisitID = ?)");
    auto queryVisitDates = m_database.prepare(R"(SELECT Date FROM Visits WHERE VisitID = ?
                                              AND Date >= ? AND Date <= ? ORDER BY Date ASC)");

    queryVisitIds << startDate
                  << endDate;
    while (queryVisitIds.next())
    {
        int visitId = 0;
        queryVisitIds >> visitId;

        queryHistoryItem.reset();
        queryHistoryItem << visitId;
        if (!queryHistoryItem.execute() || !queryHistoryItem.next())
            continue;

        HistoryEntry entry;
        entry.VisitID = visitId;
        queryHistoryItem >> entry.URL
                         >> entry.Title
                         >> entry.URLTypedCount;

        std::vector<VisitEntry> visits;
        queryVisitDates.reset();
        queryVisitDates << visitId
                        << startDate
                        << endDate;
        while (queryVisitDates.next())
        {
            QDateTime temp = QDateTime();
            queryVisitDates >> temp;
            visits.push_back(temp);
        }

        entry.LastVisit = visits.at(visits.size() - 1);
        entry.NumVisits = static_cast<int>(visits.size());
        result.push_back( URLRecord{ std::move(entry), std::move(visits) } );
    }

    return result;
}

int HistoryStore::getTimesVisitedHost(const QUrl &url) const
{
    auto query = m_database.prepare(R"(SELECT COUNT(NumVisits) FROM (SELECT VisitID, COUNT(VisitID) AS NumVisits
                                    FROM Visits INDEXED BY Visit_ID_Index GROUP BY VisitID ) WHERE VisitID IN (SELECT VisitID FROM History
                                    WHERE URL LIKE ?))");
    std::string param = QString("%%1%").arg(url.host().remove(QRegularExpression("^www\\.")).toLower()).toStdString();
    query << param;
    if (query.next())
    {
        int numVisits = 0;
        query >> numVisits;
        return numVisits;
    }

    return 0;
}

int HistoryStore::getTimesVisited(const QUrl &url) const
{
    auto query = m_database.prepare(R"(SELECT h.VisitID, v.NumVisits FROM History AS h
                                    INNER JOIN (SELECT VisitID, COUNT(VisitID) AS NumVisits
                                    FROM Visits INDEXED BY Visit_ID_Index GROUP BY VisitID) AS v
                                    ON h.VisitID = v.VisitID WHERE h.URL = ?)");
    query << url;
    if (query.next())
    {
        int visitId = 0,
            numVisits = 0;
        query >> visitId
              >> numVisits;
        return numVisits;
    }

    return 0;
}

std::map<int, QString> HistoryStore::getWords() const
{
    std::map<int, QString> result;

    auto stmt = m_database.prepare(R"(SELECT WordID, Word FROM Words ORDER BY WordID ASC)");
    if (stmt.execute())
    {
        while (stmt.next())
        {
            int wordId = 0;
            QString word;

            stmt >> wordId
                 >> word;
            result.insert(std::make_pair(wordId, word));
        }
    }

    return result;
}

std::map<int, std::vector<int>> HistoryStore::getEntryWordMapping() const
{
    std::map<int, std::vector<int>> result;

    auto queryDistinctHistoryId = m_database.prepare(R"(SELECT DISTINCT(HistoryID) FROM URLWords ORDER BY HistoryID ASC)");
    auto queryHistoryWordMapping =
            m_database.prepare(R"(SELECT WordID FROM URLWords WHERE HistoryID = ?)");

    while (queryDistinctHistoryId.next())
    {
        int historyId = 0;
        queryDistinctHistoryId >> historyId;
        //queryDistinctHistoryId.value(0).toInt();

        std::vector<int> wordIds;
        queryHistoryWordMapping.reset();
        queryHistoryWordMapping << historyId;
        while (queryHistoryWordMapping.next())
        {
            int wordId = 0;
            queryHistoryWordMapping >> wordId;
            wordIds.push_back(wordId);
        }

        if (!wordIds.empty())
            result.insert(std::make_pair(historyId, wordIds));
    }

    return result;
}

void HistoryStore::addVisit(const QUrl &url, const QString &title, const QDateTime &visitTime, const QUrl &requestedUrl, bool wasTypedByUser)
{
    if (url.toString(QUrl::FullyEncoded).startsWith(QStringLiteral("data:")))
        return;

    auto existingEntry = getEntry(url);
    qulonglong visitId = existingEntry.VisitID >= 0 ? static_cast<qulonglong>(existingEntry.VisitID) : ++m_lastVisitID;
    if (existingEntry.VisitID >= 0)
    {
        if (wasTypedByUser)
            existingEntry.URLTypedCount++;

        existingEntry.Title = title;

        sqlite::PreparedStatement &stmtUpdate = m_statements.at(Statement::UpdateHistoryRecord);
        stmtUpdate.reset();
        stmtUpdate << existingEntry;

        if (!stmtUpdate.execute())
            qWarning() << "HistoryStore::addVisit - could not save entry to database.";

        tokenizeAndSaveUrl(static_cast<int>(visitId), url, title);
    }
    else
    {
        const int urlTypedCount = wasTypedByUser ? 1 : 0;

        sqlite::PreparedStatement &stmtNew = m_statements.at(Statement::CreateHistoryRecord);
        stmtNew.reset();

        stmtNew << visitId
                << url
                << title
                << urlTypedCount;

        if (stmtNew.execute())
            tokenizeAndSaveUrl(static_cast<int>(visitId), url, title);
        else
            qWarning() << "HistoryStore::addVisit - could not save entry to database.";
    }

    sqlite::PreparedStatement &stmtVisit = m_statements.at(Statement::CreateVisitRecord);
    stmtVisit.reset();

    stmtVisit << visitId
              << visitTime;

    if (!stmtVisit.execute())
        qWarning() << "HistoryStore::addVisit - could not save visit to database.";

    if (!CommonUtil::doUrlsMatch(url, requestedUrl, true))
    {
        QDateTime requestDateTime = visitTime.addMSecs(-100);
        if (!requestDateTime.isValid())
            requestDateTime = visitTime;

        addVisit(requestedUrl, title, requestDateTime, requestedUrl, wasTypedByUser);
    }
}

uint64_t HistoryStore::getLastVisitId() const
{
    return m_lastVisitID;
}

void HistoryStore::tokenizeAndSaveUrl(int visitId, const QUrl &url, const QString &title)
{
    QStringList urlWords = CommonUtil::tokenizePossibleUrl(url.toString().toUpper());

    if (!title.startsWith(QLatin1String("http"), Qt::CaseInsensitive))
        urlWords = urlWords + title.toUpper().split(QLatin1Char(' '), QStringSplitFlag::SkipEmptyParts);

    sqlite::PreparedStatement &stmtInsertWord = m_statements.at(Statement::CreateWordRecord);
    sqlite::PreparedStatement &stmtAssociateWord = m_statements.at(Statement::CreateUrlWordRecord);

    for (const QString &word : qAsConst(urlWords))
    {
        stmtInsertWord.reset();
        stmtAssociateWord.reset();

        stmtInsertWord << word;
        stmtInsertWord.execute();

        stmtAssociateWord << visitId
                          << word;
        stmtAssociateWord.execute();
    }
}

bool HistoryStore::hasProperStructure()
{
    // Verify existence of Visits and History tables
    return hasTable(QLatin1String("Visits")) && hasTable(QLatin1String("History"));
}

void HistoryStore::setup()
{
    if (!exec(QLatin1String("CREATE TABLE IF NOT EXISTS History(VisitID INTEGER PRIMARY KEY AUTOINCREMENT, URL TEXT UNIQUE NOT NULL, Title TEXT, "
                                  "URLTypedCount INTEGER DEFAULT 0)")))
    {
        qWarning() << "In HistoryStore::setup - unable to create history table.";
    }

    if (!exec(QLatin1String("CREATE TABLE IF NOT EXISTS Visits(VisitID INTEGER NOT NULL, Date INTEGER NOT NULL, "
                                  "FOREIGN KEY(VisitID) REFERENCES History(VisitID) ON DELETE CASCADE, PRIMARY KEY(VisitID, Date))")))
    {
        qWarning() << "In HistoryStore::setup - unable to create visit table.";
    }

    if (!exec(QLatin1String("CREATE TABLE IF NOT EXISTS Words(WordID INTEGER PRIMARY KEY AUTOINCREMENT, Word TEXT NOT NULL, UNIQUE (Word))")))
    {
        qWarning() << "In HistoryStore::setup - unable to create words table.";
    }

    if (!exec(QLatin1String("CREATE TABLE IF NOT EXISTS URLWords(HistoryID INTEGER NOT NULL, WordID INTEGER NOT NULL, "
                                  "FOREIGN KEY(HistoryID) REFERENCES History(VisitID) ON DELETE CASCADE, "
                                  "FOREIGN KEY(WordID) REFERENCES Words(WordID) ON DELETE CASCADE, PRIMARY KEY(HistoryID, WordID))")))
    {
        qWarning() << "In HistoryStore::setup - unable to create url-word association table.";
    }
}

void HistoryStore::load()
{
    purgeOldEntries();
    checkForUpdate();

    if (!exec(QLatin1String("CREATE INDEX IF NOT EXISTS Visit_ID_Index ON Visits(VisitID)")))
        qWarning() << "In HistoryStore::load - unable to create index on the visit ID column of the visit table.";

    if (!exec(QLatin1String("CREATE INDEX IF NOT EXISTS Visit_Date_Index ON Visits(Date)")))
        qWarning() << "In HistoryStore::load - unable to create index on the date column of the visit table.";

    if (!exec(QLatin1String("CREATE INDEX IF NOT EXISTS Word_Index ON Words(Word)")))
        qWarning() << "In HistoryStore::load - unable to create index on the word column of the words table.";

    // Create and cache our prepared statements
    auto cacheStatement = [this](Statement statement, const std::string &sql) {
        m_statements.insert(std::make_pair(statement, m_database.prepare(sql)));
    };

    cacheStatement(Statement::CreateHistoryRecord, R"(INSERT INTO History(VisitID, URL, Title, URLTypedCount) VALUES(?, ?, ?, ?))");
    cacheStatement(Statement::UpdateHistoryRecord, R"(INSERT OR REPLACE INTO History(VisitID, URL, Title, URLTypedCount) VALUES(?, ?, ?, ?))");
    cacheStatement(Statement::CreateVisitRecord, R"(INSERT INTO Visits(VisitID, Date) VALUES (?, ?))");
    cacheStatement(Statement::CreateWordRecord, R"(INSERT OR IGNORE INTO Words(Word) VALUES (?))");
    cacheStatement(Statement::CreateUrlWordRecord, R"(INSERT OR IGNORE INTO URLWords(HistoryID, WordID) VALUES (?, (SELECT WordID FROM Words WHERE Word = ?)))");
    cacheStatement(Statement::GetHistoryRecord, "SELECT History.VisitID, History.URL, History.Title, History.URLTypedCount, V.NumVisits, "
                                                " V.RecentVisit FROM History INNER JOIN"
                                                " (SELECT VisitID, MAX(Date) AS RecentVisit, COUNT(Date) AS NumVisits "
                                                " FROM Visits INDEXED BY Visit_ID_Index GROUP BY VisitID) AS V"
                                                " ON History.VisitID = V.VisitID "
                                                " WHERE History.URL = ?");

    auto stmt = m_database.prepare(R"(SELECT MAX(VisitID) FROM History)");
    if (stmt.next())
        stmt >> m_lastVisitID;
}

void HistoryStore::checkForUpdate()
{
    // Check if table structure needs update before loading
    auto stmt = m_database.prepare(R"(PRAGMA table_info(History))");
    if (!stmt.execute())
        return;

    bool hasUrlTypeCountColumn = false;
    const QString urlTypeCountColumn("URLTypedCount");

    while (stmt.next())
    {
        int cid = 0;
        QString colName;

        stmt >> cid
             >> colName;

        if (colName.compare(urlTypeCountColumn) == 0)
        {
            hasUrlTypeCountColumn = true;
            break;
        }
    }

    if (!hasUrlTypeCountColumn)
    {
        if (!exec(QLatin1String("ALTER TABLE History ADD URLTypedCount INTEGER DEFAULT 0")))
            qDebug() << "Error updating history table with url typed count column";
    }
}

void HistoryStore::purgeOldEntries()
{
    // Clear visits that are >8 weeks old
    quint64 purgeDate = static_cast<quint64>(QDateTime::currentMSecsSinceEpoch());
    const quint64 tmp = quint64{4838400000};
    if (purgeDate > tmp)
    {
        purgeDate -= tmp;

        auto stmt = m_database.prepare(R"(DELETE FROM Visits WHERE Date < ?)");
        stmt << purgeDate;
        if (!stmt.execute())
        {
            qWarning() << "HistoryStore - Could not purge old history entries.";
        }

        if (!m_database.execute(R"(DELETE FROM History WHERE VisitID NOT IN (SELECT VisitID FROM Visits);)"))
            qWarning() << "HistoryStore - Error purging unused history entries. Message: " << QString::fromStdString(m_database.getLastError());
    }
}

std::vector<WebPageInformation> HistoryStore::loadMostVisitedEntries(int limit)
{
    std::vector<WebPageInformation> result;
    if (limit <= 0)
        return result;

    auto stmt =
            m_database.prepare(R"(SELECT v.VisitID, COUNT(v.VisitID) AS NumVisits, h.URL, h.Title
                               FROM Visits AS v
                               JOIN History AS h
                                 ON v.VisitID = h.VisitID
                               GROUP BY v.VisitID
                               ORDER BY NumVisits DESC LIMIT ?)");
    stmt << limit;
    if (!stmt.execute())
    {
        qWarning() << "In HistoryStore::loadMostVisitedEntries - unable to load most frequently visited entries.";
        return result;
    }

    int count = 0;
    while (stmt.next())
    {
        int visitId = 0, numVisits = 0;
        stmt >> visitId
             >> numVisits;

        WebPageInformation item;
        item.Position = count++;

        stmt >> item.URL
             >> item.Title;
        result.push_back(std::move(item));
    }

    return result;
}

