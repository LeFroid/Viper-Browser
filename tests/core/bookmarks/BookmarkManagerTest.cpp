#include "BookmarkManager.h"
#include "BookmarkNode.h"
#include "BookmarkStore.h"
#include "DatabaseTaskScheduler.h"
#include "ServiceLocator.h"

#include <chrono>
#include <memory>
#include <thread>

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

    void testRemovingBookmarkFromFolder();

    void testChangingBookmarkParent();

    void testBookmarkCheckWithTrailingSlash();

private:
    /// Root node/folder used in bookmark management tests
    std::shared_ptr<BookmarkNode> m_root;

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

    ViperServiceLocator serviceLocator;
    DatabaseTaskScheduler taskScheduler;
    m_manager = new BookmarkManager(serviceLocator, taskScheduler, nullptr);
    m_manager->setRootNode(m_root);
}

void BookmarkManagerTest::cleanupTestCase()
{
    if (m_manager)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_manager->waitToFinishList();
        delete m_manager;
        m_manager = nullptr;
    }
}

void BookmarkManagerTest::testAddingBookmarkToFolder()
{
    QUrl bookmarkUrl { QLatin1String("https://www.home.page/") };
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

void BookmarkManagerTest::testRemovingBookmarkFromFolder()
{
    QUrl bookmarkUrl { QLatin1String("https://some.website.com/welcome") };

    // Add bookmark, test delete by URL
    m_manager->appendBookmark(QLatin1String("To Delete"), bookmarkUrl, m_root.get());
    QVERIFY2(m_manager->isBookmarked(bookmarkUrl), "Bookmark manager should have inserted the bookmark into the collection");

    m_manager->removeBookmark(bookmarkUrl);
    QVERIFY2(!m_manager->isBookmarked(bookmarkUrl), "Bookmark manager should have removed the bookmark from the collection");

    // Add bookmark, test delete by Node pointer
    bookmarkUrl = QUrl(QLatin1String("http://othersite.net/forum"));
    m_manager->appendBookmark(QLatin1String("To Delete"), bookmarkUrl, m_root.get());

    BookmarkNode *node = m_manager->getBookmark(bookmarkUrl);
    QVERIFY2(node != nullptr, "Bookmark should exist");

    if (node)
    {
        m_manager->removeBookmark(node);
        QVERIFY2(!m_manager->isBookmarked(bookmarkUrl), "Bookmark manager should have deleted the node");
    }

    // Add folder, test delete by Node pointer
    const auto folderName = QLatin1String("My Folder");
    node = m_manager->addFolder(folderName, m_root.get());
    QVERIFY2(node != nullptr, "Folder should exist");
    if (node)
    {
        m_manager->removeBookmark(node);

        bool folderStillExists = false;
        for (int i = 0; i < m_root->getNumChildren(); ++i)
        {
            BookmarkNode *child = m_root->getNode(i);
            if (child->getType() == BookmarkNode::Folder && child->getName() == folderName)
                folderStillExists = true;
        }
        QVERIFY2(!folderStillExists, "Bookmark manager should have deleted the node");
    }
}

void BookmarkManagerTest::testChangingBookmarkParent()
{
    QUrl bookmarkUrl { QLatin1String("https://viper-browser.com/") };

    // Add bookmark to root folder
    m_manager->appendBookmark(QLatin1String("Viper Browser"), bookmarkUrl, m_root.get());
    BookmarkNode *bookmark = m_manager->getBookmark(bookmarkUrl);
    QVERIFY2(bookmark != nullptr, "Bookmark manager should have inserted the bookmark into the collection");
    QVERIFY2(bookmark->getParent() == m_root.get(), "Bookmark parent should be root folder");

    // Create a sub-folder
    BookmarkNode *linkFolder = m_manager->addFolder(QLatin1String("Links"), m_root.get());
    QVERIFY2(linkFolder != nullptr, "Folder should exist");

    bookmark = m_manager->setBookmarkParent(bookmark, linkFolder);
    QVERIFY2(bookmark != nullptr, "Bookmark should not be null after changing parent");
    QVERIFY2(bookmark->getParent() == linkFolder, "Bookmark parent should be link folder");

    // Create another folder and move the "Links" folder into it
    BookmarkNode *miscFolder = m_manager->addFolder(QLatin1String("Misc"), m_root.get());
    QVERIFY2(miscFolder != nullptr, "Folder should exist");

    linkFolder = m_manager->setBookmarkParent(linkFolder, miscFolder);
    QVERIFY2(linkFolder != nullptr, "Folder should exist");
    QVERIFY2(linkFolder->getParent() == miscFolder, "Link folder parent should be Misc folder");
    QVERIFY2(bookmark->getParent() == linkFolder, "Bookmark parent should be link folder");

    QVERIFY2(miscFolder->getParent() == m_root.get(), "Misc folder parent should be root");

    // Verify that we can't set the parent of miscFolder to its child (Link folder)
    miscFolder = m_manager->setBookmarkParent(miscFolder, linkFolder);
    QVERIFY2(miscFolder->getParent() == m_root.get(), "Misc folder parent should be root");
    QVERIFY2(linkFolder->getParent() == miscFolder, "Link folder parent should be Misc folder");
}

void BookmarkManagerTest::testBookmarkCheckWithTrailingSlash()
{
    QUrl bookmarkUrl { QLatin1String("https://viper-browser.com/") };
    QUrl compareToUrl { QLatin1String("https://viper-browser.com") };

    m_manager->appendBookmark(QLatin1String("Viper Browser"), bookmarkUrl, m_root.get());
    QVERIFY2(m_manager->isBookmarked(compareToUrl), "Bookmark manager should ignore trailing slashes when checking if a URL is bookmarked");
}

QTEST_APPLESS_MAIN(BookmarkManagerTest)

#include "BookmarkManagerTest.moc"
