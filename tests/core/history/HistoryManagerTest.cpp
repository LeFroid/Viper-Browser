#include "DatabaseFactory.h"
#include "HistoryManager.h"

#include <QObject>
#include <QString>
#include <QTest>

class HistoryManagerTest : public QObject
{
    Q_OBJECT

public:
    HistoryManagerTest() :
        QObject(nullptr)
    {
    }
};

QTEST_APPLESS_MAIN(HistoryManagerTest)

#include "HistoryManagerTest.moc"

