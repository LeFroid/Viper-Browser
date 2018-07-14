#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SecurityManager.h"

#include <memory>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

#ifdef Q_OS_LINUX

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <iostream>
#include <execinfo.h>
#include <signal.h>

#define BT_BUF_SIZE 100

void handleCrash(int signum)
{
    if (signum != SIGSEGV)
        return;

    void *buffer[BT_BUF_SIZE];

    int nptrs = backtrace(buffer, BT_BUF_SIZE);
    char **strings = backtrace_symbols(buffer, nptrs);

    if (strings == NULL)
    {
        std::cout << "Could not get backtrace symbols" << std::endl;
        exit(EXIT_FAILURE);
    }

    QDir d(QString("%1/.config/Vaccarelli").arg(QDir::homePath()));
    if (!d.cd(QLatin1String("crash-logs")))
    {
        if (!d.mkdir(QLatin1String("crash-logs")) || !d.cd(QLatin1String("crash-logs")))
        {
            std::cout << "Could not create crash log directory" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    QDateTime now = QDateTime::currentDateTime();
    QFile crashFile(QString("%1/Crash_%2.log").arg(d.absolutePath()).arg(now.toString(Qt::ISODate)));
    if (crashFile.open(QIODevice::WriteOnly))
    {
        QTextStream stream(&crashFile);
        for (int i = 0; i < nptrs; ++i)
        {
            stream << "[" << i << "]: " << strings[i] << "\n";
        }

        crashFile.close();

        std::cout << "Saved crash log to " << QFileInfo(crashFile).absoluteFilePath().toStdString() << std::endl;
    }
    else
        std::cout << "Could not save crash log to file" << std::endl;

    free(strings);
}

#undef BT_BUF_SIZE

#endif

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
    signal(SIGSEGV, handleCrash);
#endif

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    BrowserApplication a(argc, argv);
#else
    int argc2 = 2;
    char *argv2[] = { argv[0], "--remote-debugging-port=9477" };
    BrowserApplication a(argc2, argv2);
#endif

    static_cast<void>(a.getNewWindow());

    return a.exec();
}
