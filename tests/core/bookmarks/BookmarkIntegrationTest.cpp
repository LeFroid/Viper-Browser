#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BookmarkStore.h"
#include "CommonUtil.h"
#include "DatabaseFactory.h"
#include "DatabaseTaskScheduler.h"
#include "ServiceLocator.h"

#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include <QFile>
#include <QObject>
#include <QString>
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

    /// Verifies that a change in a bookmark's name, url, shortcut, etc. can be persisted across
    /// multiple sessions
    void testModificationsPersisted();

    /// Adds a bookmark to a folder, deletes the parent folder and then checks if the bookmark manager
    /// still thinks the node is bookmarked
    void testIsBookmarkedAfterDeletingParentFolder();

private:
    /// Bookmark database file used for testing
    QString m_dbFile;

    /// Points to the bookmark manager, which is instantiated by the bookmark store
    BookmarkManager *m_bookmarkManager;

    /// Task scheduler
    std::unique_ptr<DatabaseTaskScheduler> m_taskScheduler;
};

BookmarkIntegrationTest::BookmarkIntegrationTest() :
    QObject(nullptr),
    m_dbFile(QLatin1String("BookmarkIntegrationTest.db")),
    m_bookmarkManager(nullptr),
    m_taskScheduler(nullptr)
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
    m_taskScheduler = std::make_unique<DatabaseTaskScheduler>();
    m_taskScheduler->addWorker("BookmarkStore", std::bind(DatabaseFactory::createDBWorker<BookmarkStore>, m_dbFile));

    m_bookmarkManager = new BookmarkManager(emptyServiceLocator, *m_taskScheduler, nullptr);
    m_taskScheduler->run();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
}

void BookmarkIntegrationTest::cleanup()
{
    m_taskScheduler->stop();

    delete m_bookmarkManager;
    m_bookmarkManager = nullptr;

    m_taskScheduler.reset(nullptr);
}

void BookmarkIntegrationTest::cleanupTestCase()
{
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

    BookmarkNode *lastFolder = root->getNode(root->getNumChildren() - 1);
    QVERIFY2(lastFolder && lastFolder->getName().compare(names.at(names.size() - 1)) == 0,
             "Should be able to retrieve the Programming folder from the root of the bookmark tree");

    // Used in next test
    m_bookmarkManager->appendBookmark(QLatin1String("Link A"), QUrl(QLatin1String("https://link-a.net/")), lastFolder);
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

    // Verify child bookmark data is same as before
    BookmarkNode *lastFolder = root->getNode(root->getNumChildren() - 1);
    QVERIFY2(lastFolder && lastFolder->getName().compare(names.at(names.size() - 1)) == 0,
             "Should be able to retrieve the Programming folder from the root of the bookmark tree");
    QVERIFY2(lastFolder->getNumChildren() == 1, "Programming folder should have 1 child node");

    BookmarkNode *child = lastFolder->getNode(0);
    QVERIFY2(child->getName().compare(QLatin1String("Link A")) == 0, "Bookmark name should persist across sessions");
    QVERIFY2(child->getType() == BookmarkNode::Bookmark, "Bookmark type should persist across sessions");

    const QUrl expectedUrl { QLatin1String("https://link-a.net/") };
    QVERIFY2(CommonUtil::doUrlsMatch(child->getURL(), expectedUrl), "Bookmark URL should persist across sessions");

    // Now change URL and shortcut for next test
    m_bookmarkManager->setBookmarkURL(child, QUrl(QLatin1String("https://link-abc.org/")));
    m_bookmarkManager->setBookmarkShortcut(child, QLatin1String("abc"));
}

void BookmarkIntegrationTest::testModificationsPersisted()
{
    BookmarkNode *root = m_bookmarkManager->getRoot();
    QVERIFY(root != nullptr);
    QCOMPARE(root->getNumChildren(), 4);

    BookmarkNode *lastFolder = root->getNode(root->getNumChildren() - 1);
    QVERIFY(lastFolder != nullptr);

    BookmarkNode *child = lastFolder->getNode(0);
    QVERIFY(child != nullptr);

    const QUrl expectedUrl { QLatin1String("https://link-abc.org/") };
    QVERIFY2(CommonUtil::doUrlsMatch(child->getURL(), expectedUrl), "Change in bookmark's URL should be persisted");
    QVERIFY2(child->getShortcut().compare(QLatin1String("abc")) == 0, "Change in bookmark's shortcut should be persisted");
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
