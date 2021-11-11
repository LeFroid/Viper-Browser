#include "AppInitSettings.h"
#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "BrowserIPC.h"
#include "MainWindow.h"
#include "SecurityManager.h"
#include "SchemeRegistry.h"
#include "URLSuggestion.h"
#include "WebWidget.h"
#include "ui/welcome_window/WelcomeWindow.h"
#include "config.h"

#include <memory>
#include <vector>
#include <QMetaType>
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
    if (s != SIGSEGV)
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
    QFile crashFile(QString("%1/Crash_%2.log").arg(d.absolutePath(), now.toString(Qt::ISODate)));
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

    exit(EXIT_FAILURE);
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
    // Check for version flag - we will only print the version of the program and exit when this is specified
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (strcmp(argv[i], "-v") == 0
                    || strcmp(argv[i], "--version") == 0)
            {
                std::cout << "Viper-Browser " << VIPER_VERSION_STR << ", written by Timothy Vaccarelli." << std::endl;
                std::cout << "For more information visit https://github.com/LeFroid/Viper-Browser" << std::endl;
                return 0;
            }
        }
    }

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

    // Check if any application arguments include URLs
    std::vector<QUrl> appArgUrls;
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            QString arg(argv[i]);
            QUrl url = QUrl::fromUserInput(arg);
            if (!url.isEmpty() && !url.scheme().isEmpty() && url.isValid())
                appArgUrls.push_back(url);
        }
    }

    // Check if there is an existing instance of the browser application
    BrowserIPC ipc;
    if (ipc.hasExistingInstance())
    {
        QString ipcMessage;

        // If appArgUrls is not empty, concatenate the
        // URLs into a string (length must be < BrowserIPC::BufferLength),
        // and pass as a single message to existing process.
        // Otherwise, pass message "new-window"
        if (!appArgUrls.empty())
        {
            const int headerLen = static_cast<int>(sizeof(int));
            for (size_t i = 0; i < appArgUrls.size(); ++i)
            {
                const QUrl &url = appArgUrls.at(i);
                const QString urlString = url.toString();
                const bool isLastUrl = i + 1 == appArgUrls.size();

                int additionalLength = urlString.length();
                if (!isLastUrl)
                    additionalLength++;

                if (ipcMessage.length() + additionalLength + headerLen > BrowserIPC::BufferLength)
                    break;

                ipcMessage += urlString;
                if (!isLastUrl)
                    ipcMessage += QLatin1Char('\t');
            }
        }
        else
        {
            ipcMessage = QLatin1String("new-window");
        }

        std::string ipcMessageStdString = ipcMessage.toStdString();

        size_t ipcBufferLen = ipcMessageStdString.size() + sizeof(int);

        std::vector<char> messageBuffer(ipcBufferLen, '\0');
        char *rawBuffer = messageBuffer.data();
        const int stringLen = static_cast<int>(ipcMessageStdString.size());
        memcpy(rawBuffer, &stringLen, sizeof(int));
        memcpy(&rawBuffer[sizeof(int)], ipcMessageStdString.c_str(), ipcMessageStdString.size());

        ipc.sendMessage(const_cast<const char*>(rawBuffer), static_cast<int>(ipcBufferLen));

        return 0;
    }

    qRegisterMetaType<URLSuggestion>();
    qRegisterMetaType<std::vector<URLSuggestion>>();

    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::Floor);

    AppInitSettings initSettings;

    std::string emptyStr = "";
    std::vector<char> emptyStrBuffer(emptyStr.begin(), emptyStr.end());

    int argc2 = 1;
    char *argv2[] = { argv[0], emptyStrBuffer.data(), emptyStrBuffer.data() };

    std::string disableGpu = initSettings.getValue(AppInitKey::DisableGPU);
    std::vector<char> disableGpuBuffer(disableGpu.begin(), disableGpu.end());
    disableGpuBuffer.resize(disableGpu.size() + 1);

    std::string processModel = initSettings.getValue(AppInitKey::ProcessModel);
    std::vector<char> processModelBuffer(processModel.begin(), processModel.end());
    processModelBuffer.resize(processModel.size() + 1);

    if (!disableGpu.empty())
    {
        disableGpuBuffer[disableGpu.size()] = '\0';
        argv2[argc2++] = disableGpuBuffer.data();
    }
    if (!processModel.empty())
    {
        processModelBuffer[processModel.size()] = '\0';
        argv2[argc2++] = processModelBuffer.data();
    }

    BrowserApplication a(&ipc, argc2, argv2);

    if (!initSettings.hasSetting(AppInitKey::CompletedInitialSetup)
            || initSettings.getValue(AppInitKey::CompletedInitialSetup).size() < 3)
    {
        initSettings.setValue(AppInitKey::CompletedInitialSetup, "yes");

        MainWindow *mainWindow = a.getNewWindow();
        mainWindow->show();

        WelcomeWindow welcomeWindow(a.getSettings(), a.getAdBlockManager());
        welcomeWindow.show();

        return a.exec();
    }

    MainWindow *window = a.getNewWindow();
    if (!appArgUrls.empty())
    {
        for (const QUrl &url : appArgUrls)
        {
            if (window->currentWebWidget()->isOnBlankPage())
                window->loadUrl(url);
            else
                window->openLinkNewTab(url);
        }
    }

    return a.exec();
}
