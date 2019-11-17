#include "CommonUtil.h"
#include "DatabaseFactory.h"
#include "FaviconManager.h"
#include "NetworkAccessManager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QTest>

class FaviconManagerTest : public QObject
{
    Q_OBJECT

public:
    FaviconManagerTest() :
        QObject(nullptr),
        m_dbFile(QLatin1String("FaviconManagerTest.db")),
        m_faviconManager(nullptr)
    {
    }

private slots:
    /// Called before any tests are executed
    void initTestCase()
    {
        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
    }

    /// Called after every test function. This closes the DB connection and deallocates / clears
    /// the HistoryStore instance
    void cleanup()
    {
        if (m_faviconManager)
        {
            delete m_faviconManager;
            m_faviconManager = nullptr;
        }

        if (QFile::exists(m_dbFile))
            QFile::remove(m_dbFile);
    }

    void testCanAddAndThenFetchIcon()
    {
        NetworkAccessManager accessManager;

        m_faviconManager = new FaviconManager(QLatin1String("FaviconManagerTest.db"));
        m_faviconManager->setNetworkAccessManager(&accessManager);

        QString iconEncoded = QStringLiteral("iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAACXBIWXMAAA1hAAAMxAHulkC1AAAEmklEQVRYha1XX2hbVRz+vnNv02xrIxu9N1mSlTiuIL26PdStiMjciyjq9ElkTwMfrMhEdOjDYEunU/BBJw71QXwUpA+WoQznZGygOOdEO6aCQWKbP7e5rptpM9qkyc+HJttdctNma76nnN/vnO/7zrknv3MO0SFs2w7Muu5uAfZAZAhAVMgoAFAkByAH8ncCJzYZxpnLly+XO+Hlah0ShhFZIA+LyF4AoQ79Fkl+HhQZS7uuc0cGLMvqnS8WDwJ4VUQ2dCh8KzlZAvBeXyh0NJVKLXZsoD7rCREZuRNhHyPngyLP+K1Gi4G4aW5bEvlagHg3xD1CGZ18IlMoTLY1kDCMyAJwodviXhNBYId3JVTjh2VZvQvkhFecwB8EviB55Q70rhEYJ/BLIyBAfIGcsCyrt8XAfLF4sPmbU6ljjus+d+/QUARKPQ9y2Tk5D/ISgXMEzoKcBPDfcoqzBPYPmGbYcd1nSR71corISH1zNyZ5Y9Olmnc7NW2n4zgXGm3bMPquaNrg6Ojon8lkstZEzHg8fo+mae7U1NTVRjwcDt+NWu3vW3jJUlDESruuQwCImObHIjLavIZK1x/I5/MX/da3U0Sj0cFqpfJPc5zkJ06h8KKybTtQLzKtqFaH1iIOALVazfaLi8he27YDatZ1d8OnwpEsQdPOrdWApmk/kfzXJxWadd3dSoA9bcYeyefzLUt3u8hms1cg8rpfToA9qn6wtEAPBL5cq3gDwQ0b/LlEhhSAaHOcZGl6ejrVLQPpdPoaySmfVFQ1jtQmFEhKtwwAAERmWkJkVNFTjG72lYGuigMA2cJJQCkAfju0PxwOm93STiQSQfh8agCuAuB7YVAij3bLwOL167tEpNcn5SiK/Og3SEReFpFVb0ydQIBX/OIUOa9Anm0zaMfmcPi1tYpHDGOfiDzmmyTPqtDGjd8CmPM1IfJuxDSPDA8P99yucDKZVBHTPCDAp226zG0SOdU4jI6LyEt1V+9r5M9VkTcgsg0ACEyD/EwB36tA4GImk5n1Y9y6detdpVJpWAEP1kT2QcRqZ5BKHXdmZvYrANBFjpEsAwCXB7oDIg+B/BUABNgiIoerIqcq5fKJZDLZ8tcFgOtzcxOo1b6r1WpvrShOLvaIHKtPbhkR0xwTkUP1ZlEPBGyS65bK5R+8dUFT6unczMwJP+JIJPKIVKtn2gl7DIw5hUIS8BShvlDo7frNBgBC1XJ5LJPJ/BUUuZ/kAQAfKKVeWN/f/007Yl3XJ9vlPOqTfaHQOzea3lw0Gt1SrVTOA9gMoEpNe8pxnJOrknoQNowKAL1NOq/19IzkcrlpXwMAEDPN7UsiJ2+YIE8DOE1gToDww7t2HR0fH6+uYGAJgOYnrpOPZwuF37xB30ITi8XiS5XKVxDZ3pwbMM3eld59YcOoovl8IS9puv5kLpdrORF9d3M2m830h0IjJMdI3vKkKpVKvmO8cjd1WSb55rr163f6ibc1AACpVGrRKRSSPYBNpT4EUAQ5n0gkllZUJ6+CnCf5kR4I3OcUCofS6fTCKqZXh20YfYODgxtX6xeLxeKWZXX6isb/mQzVddO1ixsAAAAASUVORK5CYII=");
        QIcon icon = CommonUtil::iconFromBase64(iconEncoded.toLatin1());
        QVERIFY(!icon.isNull());

        QString iconUrl = QLatin1String("https://assets-cdn.github.com/favicon.ico");
        QUrl pageUrl = QUrl::fromUserInput(QLatin1String("https://github.com/LeFroid/Viper-Browser"));

        m_faviconManager->updateIcon(iconUrl, pageUrl, icon);
        QIcon favicon = m_faviconManager->getFavicon(pageUrl);
        QVERIFY(!favicon.isNull());

        QByteArray faviconBytes = CommonUtil::iconToBase64(favicon);

        auto hashOrig = QCryptographicHash::hash(CommonUtil::iconToBase64(icon), QCryptographicHash::Sha256);
        auto hashNew = QCryptographicHash::hash(faviconBytes, QCryptographicHash::Sha256);
        QCOMPARE(hashOrig, hashNew);
        QTest::qWait(500);

        m_faviconManager->setNetworkAccessManager(nullptr);
    }

    void testCanDownloadIconFromUrl()
    {
        //todo: this
    }

    /* favicon mgr definition
    explicit FaviconManager(DatabaseTaskScheduler &taskScheduler);

    void setNetworkAccessManager(NetworkAccessManager *networkAccessManager);

    void getFavicon(const QUrl &url, std::function<void(QIcon)> callback);

    void updateIcon(const QString &iconHRef, const QUrl &pageUrl, const QIcon &pageIcon);*/

private:
    /// Database file name used for tests
    QString m_dbFile;

    /// Points to the favicon manager. This is recreated on every test case.
    /// Must instantiate this on heap since its dependencies must be created before this,
    /// yet the favicon manager must also be the last thing to be destroyed after a test
    FaviconManager *m_faviconManager;
};

QTEST_MAIN(FaviconManagerTest)

#include "FaviconManagerTest.moc"
