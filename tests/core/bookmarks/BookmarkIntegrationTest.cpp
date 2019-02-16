#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BookmarkStore.h"
#include "DatabaseFactory.h"
#include "ServiceLocator.h"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <QFile>
#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QTest>
#include <QUrl>

/// Tests the interactions between the \ref BookmarkManager and the \ref BookmarkStore
/// classes to ensure they are working together as intended
class BookmarkIntegrationTest : public QObject
{
    Q_OBJECT

public:
    /// Constructs the test object
    BookmarkIntegrationTest();

private slots:
    /// Called before any tests are ran
    void initTestCase();

    /// Called before each test function is executed - this will instantiate the BookmarkStore
    /// as well as the BookmarkManager
    void init();

    /// Called after every test function. This closes the DB connection and deallocates / clears
    /// the instances of the BookmarkStore and BookmarkManager used in the prior test case
    void cleanup();

    /// Deletes the test database file after the last test case has executed
    void cleanupTestCase();

    /// Attempts to create a few folders that are direct children of the root bookmark node.
    void testAddingFoldersToRoot();

    /// Verifies that the folders added in the previous test case, testAddingFoldersToRoot(), have
    /// been saved to the database and subsequently loaded when the bookmark store was reinstantiated
    void testLoadingFoldersFromDatabase();

    /// Adds a bookmark to a folder, deletes the parent folder and then checks if the bookmark manager
    /// still thinks the node is bookmarked
    void testIsBookmarkedAfterDeletingParentFolder();

private:
    /// Bookmark database file used for testing
    QString m_dbFile;

    /// Pointer to the bookmark store
    std::unique_ptr<BookmarkStore> m_bookmarkStore;

    /// Points to the bookmark manager, which is instantiated by the bookmark store
    BookmarkManager *m_bookmarkManager;
};

BookmarkIntegrationTest::BookmarkIntegrationTest() :
    QObject(nullptr),
    m_dbFile(QLatin1String("BookmarkIntegrationTest.db")),
    m_bookmarkStore(nullptr),
    m_bookmarkManager(nullptr)
{
}

void BookmarkIntegrationTest::initTestCase()
{
    if (QFile::exists(m_dbFile))
        QFile::remove(m_dbFile);
}

void BookmarkIntegrationTest::init()
{
    ViperServiceLocator emptyServiceLocator;
    m_bookmarkStore = DatabaseFactory::createWorker<BookmarkStore>(emptyServiceLocator, m_dbFile);
    m_bookmarkManager = m_bookmarkStore->getNodeManager();
}

void BookmarkIntegrationTest::cleanup()
{
    m_bookmarkStore.reset();
    m_bookmarkManager = nullptr;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    QSqlDatabase::removeDatabase(QLatin1String("Bookmarks"));
}

void BookmarkIntegrationTest::cleanupTestCase()
{
    if (m_bookmarkStore.get())
        cleanup();

    if (QFile::exists(m_dbFile))
        QFile::remove(m_dbFile);
}

void BookmarkIntegrationTest::testAddingFoldersToRoot()
{
    QVERIFY(m_bookmarkManager != nullptr);

    const std::vector<QString> names { QLatin1String("Reading List"),
                QLatin1String("Shopping"),
                QLatin1String("Programming") };

    BookmarkNode *root = m_bookmarkManager->getRoot();
    QVERIFY(root != nullptr);

    for (const QString &name : names)
    {
        BookmarkNode *folder = m_bookmarkManager->addFolder(name, root);
        QVERIFY2(folder != nullptr, "Bookmark manager should have been able to create a folder");
    }
}

void BookmarkIntegrationTest::testLoadingFoldersFromDatabase()
{
    const std::vector<QString> names { QLatin1String("Reading List"),
                QLatin1String("Shopping"),
                QLatin1String("Programming") };

    BookmarkNode *root = m_bookmarkManager->getRoot();
    QVERIFY(root != nullptr);

    // Root should have 4 children - bookmarks bar + the three folders from last test case
    QCOMPARE(root->getNumChildren(), 4);

    for (int i = 1; i < 4; ++i)
    {
        BookmarkNode *node = root->getNode(i);
        QVERIFY(node != nullptr);
        QCOMPARE(node->getName(), names.at(i - 1));
        QVERIFY(node->getType() == BookmarkNode::Folder);
    }
}

void BookmarkIntegrationTest::testIsBookmarkedAfterDeletingParentFolder()
{
    BookmarkNode *root = m_bookmarkManager->getRoot();
    QVERIFY(root != nullptr);

    // Root should have 4 children - bookmarks bar + the three folders from last test case
    QCOMPARE(root->getNumChildren(), 4);

    BookmarkNode *subFolder = root->getNode(1);
    QCOMPARE(subFolder->getType(), BookmarkNode::Folder);

    const QUrl testUrl(QLatin1String("https://www.viper-browser.com/"));
    m_bookmarkManager->appendBookmark(QLatin1String("Viper Browser"), testUrl, subFolder);

    QVERIFY(m_bookmarkManager->isBookmarked(testUrl));
    QCOMPARE(m_bookmarkManager->getBookmark(testUrl)->getParent(), subFolder);

    m_bookmarkManager->removeBookmark(subFolder);

    QVERIFY(!m_bookmarkManager->isBookmarked(testUrl));
}

QTEST_GUILESS_MAIN(BookmarkIntegrationTest)

#include "BookmarkIntegrationTest.moc"
