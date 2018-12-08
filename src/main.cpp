#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SecurityManager.h"
#include "SchemeRegistry.h"
#include "WebWidget.h"

#include <memory>
#include <QUrl>
#include <QtGlobal>
#include <QtWebEngineCoreVersion>

#ifndef Q_OS_WIN
#include <stdio.h>
#include <stdlib.h>
#endif

#ifdef Q_OS_LINUX

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <iostream>
#include <execinfo.h>
#include <signal.h>
#include <string.h>

#define BT_BUF_SIZE 100

void _handleCrash(int s, siginfo_t * /*siginfo*/, void * /*context*/)
{
    std::cout << "handleCrash (1)\n";
    if (s != SIGSEGV)
        return;
    std::cout << "handleCrash (2)\n";
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

#ifndef Q_OS_WIN

// from http://doc.qt.io/qt-5/qtglobal.html
void viperQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type)
    {
        case QtDebugMsg:
            fprintf(stderr, "[Debug]: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtInfoMsg:
            fprintf(stderr, "[Info]: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtWarningMsg:
            fprintf(stderr, "[Warning]: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtCriticalMsg:
            fprintf(stderr, "[Critical]: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            break;
        case QtFatalMsg:
            fprintf(stderr, "[Fatal]: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
            abort();
    }
}

#endif

int main(int argc, char *argv[])
{
#ifndef Q_OS_WIN
    qInstallMessageHandler(&viperQtMessageHandler);
#endif

#ifdef Q_OS_LINUX
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_sigaction = &_handleCrash;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGSEGV, &act, NULL) < 0)
        qWarning() << "Could not install handler for signal SIGSEGV";
#endif

    SchemeRegistry::registerSchemes();

#if (QTWEBENGINECORE_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    BrowserApplication a(argc, argv);
#else
    int argc2 = 2;
    char *argv2[] = { argv[0], "--remote-debugging-port=9477" };
    BrowserApplication a(argc2, argv2);
#endif

    MainWindow *window = a.getNewWindow();
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            QString arg(argv[i]);
            QUrl url = QUrl::fromUserInput(arg);
            if (!url.isEmpty() && !url.scheme().isEmpty() && url.isValid())
            {
                if (window->currentWebWidget()->isOnBlankPage())
                    window->loadUrl(url);
                else
                    window->openLinkNewTab(url);
            }
        }
    }

    return a.exec();
}
