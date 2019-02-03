#include "BookmarkManager.h"
#include "BookmarkNode.h"

#include <memory>

#include <QObject>
#include <QString>
#include <QTest>

/// Tests the functionality of the BookmarkManager class
class BookmarkManagerTest : public QObject
{
    Q_OBJECT

public:
    BookmarkManagerTest();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testAddingBookmarkToFolder();

    void testAddingBookmarksToSubFolder_data();
    void testAddingBookmarksToSubFolder();

private:
    /// Root node/folder used in bookmark management tests
    std::unique_ptr<BookmarkNode> m_root;

    /// Bookmark manager used in test cases
    BookmarkManager *m_manager;

    /// Stores a pointer to a subfolder, used for any tests involving subfolders of the root node
    BookmarkNode *m_subFolder;
};

BookmarkManagerTest::BookmarkManagerTest() :
    QObject(nullptr),
    m_root{nullptr},
    m_manager{nullptr},
    m_subFolder{nullptr}
{
}

void BookmarkManagerTest::initTestCase()
{
    m_root.reset(new BookmarkNode(BookmarkNode::Folder, QLatin1String("Root Folder")));

    m_manager = new BookmarkManager(nullptr, nullptr);
    m_manager->setRootNode(m_root.get());
}

void BookmarkManagerTest::cleanupTestCase()
{
    delete m_manager;
    m_manager = nullptr;
}

void BookmarkManagerTest::testAddingBookmarkToFolder()
{
    QUrl bookmarkUrl { QLatin1String("https://www.home.page/") };
    qDebug() << bookmarkUrl;
    m_manager->appendBookmark(QLatin1String("Home"), bookmarkUrl, m_root.get());

    QCOMPARE(m_root->getNumChildren(), 1);

    const bool hasBookmarkWithUrl = m_manager->isBookmarked(bookmarkUrl);
    QVERIFY2(hasBookmarkWithUrl, "Bookmark manager should have inserted the bookmark into the collection");

    BookmarkNode *node = m_manager->getBookmark(bookmarkUrl);
    QVERIFY2(node != nullptr, "Bookmark node should not be null");

    cleanupTestCase();
    initTestCase();
}

void BookmarkManagerTest::testAddingBookmarksToSubFolder_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("type");
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<int>("position");

    QTest::newRow("folder") << "Watch Later" << static_cast<int>(BookmarkNode::Folder) << QUrl() << 0;

    QTest::newRow("bookmark 1") << "A" << static_cast<int>(BookmarkNode::Bookmark)
                                << QUrl("https://video.somecdn.com/watch_vid_1") << 0;
    QTest::newRow("bookmark 2") << "B" << static_cast<int>(BookmarkNode::Bookmark)
                                << QUrl("https://www.videos.net/watch_vid_2") << 1;
    QTest::newRow("bookmark 3") << "C" << static_cast<int>(BookmarkNode::Bookmark)
                                << QUrl("https://a.third.site.com/watch_vid_3") << 1;
}

void BookmarkManagerTest::testAddingBookmarksToSubFolder()
{
    QFETCH(QString, name);
    QFETCH(int, type);
    QFETCH(QUrl, url);
    QFETCH(int, position);

    switch (static_cast<BookmarkNode::NodeType>(type))
    {
        case BookmarkNode::Folder:
        {
            BookmarkNode *folder = m_manager->addFolder(name, m_root.get());

            QVERIFY2(folder != nullptr, "Subfolder not be null");
            QCOMPARE(folder->getName(), name);

            m_subFolder = folder;
            break;
        }
        case BookmarkNode::Bookmark:
        {
            m_manager->insertBookmark(name, url, m_subFolder, position);
            QVERIFY2(m_manager->isBookmarked(url), "Bookmark manager should have inserted the bookmark into the collection");

            BookmarkNode *node = m_manager->getBookmark(url);
            QVERIFY2(node != nullptr, "Bookmark node should not be null");
            if (node)
            {
                QVERIFY2(node->getParent() == m_subFolder, "Bookmark's parent should be the target subfolder");
                QVERIFY2(node->getName() == name, "Bookmark name should match");
                QVERIFY2(node->getURL() == url, "Bookmark url should match");
                QVERIFY2(node->getPosition() == position, "Bookmark position should match");
            }
            break;
        }
        default:
            break;
    }
}

QTEST_APPLESS_MAIN(BookmarkManagerTest)

#include "BookmarkManagerTest.moc"
