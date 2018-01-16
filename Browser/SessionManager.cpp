#include "SessionManager.h"
#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "WebView.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>

SessionManager::SessionManager() :
    m_dataFile(),
    m_savedSession(false)
{
}

bool SessionManager::alreadySaved() const
{
    return m_savedSession;
}

void SessionManager::setSessionFile(const QString &fullPath)
{
    m_dataFile = fullPath;
}

void SessionManager::saveState(std::vector<MainWindow*> &windows)
{
    QFile dataFile(m_dataFile);
    if (!dataFile.open(QIODevice::WriteOnly))
        return;

    // Construct a QJsonDocument containing an array of windows corresponding to the container passed to this method
    QJsonObject sessionObj;
    QJsonArray windowArray;
    for (MainWindow *win : windows)
    {
        QJsonObject winObj;
        QJsonArray tabArray;

        // Add each tab's URL to the array
        BrowserTabWidget *tabWidget = win->getTabWidget();
        int numTabs = tabWidget->count();
        for (int i = 0; i < numTabs; ++i)
        {
            WebView *view = tabWidget->getWebView(i);
            if (!view)
                continue;

            tabArray.append(QJsonValue(view->url().toString()));
        }

        winObj.insert(QString("tabs"), QJsonValue(tabArray));
        winObj.insert(QString("current_tab"), QJsonValue(tabWidget->currentIndex()));
        windowArray.append(QJsonValue(winObj));
    }
    sessionObj.insert(QString("windows"), QJsonValue(windowArray));

    // Save to file
    QJsonDocument sessionDoc(sessionObj);
    dataFile.write(sessionDoc.toJson());
    dataFile.close();

    m_savedSession = true;
}

void SessionManager::restoreSession(MainWindow *firstWindow)
{
    /* browsing session data is of the following format:
     * {
     *     "windows": [
     *         {
     *             "tabs": [ "url 1", "url 2", ... ]
     *             "current_tab": n
     *         },
     *         { ... }, ..., { ... }
     *     ]
     */

    QFile dataFile(m_dataFile);
    if (!dataFile.exists() || !dataFile.open(QIODevice::ReadOnly))
        return;

    // Attempt to parse data file
    QByteArray sessionData = dataFile.readAll();
    dataFile.close();

    QJsonDocument sessionDoc(QJsonDocument::fromJson(sessionData));
    if (!sessionDoc.isObject())
        return;

    QJsonObject sessionObj = sessionDoc.object();
    auto it = sessionObj.find("windows");
    if (it == sessionObj.end())
        return;

    // Load each tab into the appropriate windows
    bool isFirstWindow = true;
    MainWindow *currentWindow = firstWindow;

    QJsonArray winArray = it.value().toArray();
    for (auto winIt = winArray.constBegin(); winIt != winArray.constEnd(); ++winIt)
    {
        if (!isFirstWindow)
            currentWindow = sBrowserApplication->getNewWindow();
        else
            isFirstWindow = false;

        BrowserTabWidget *tabWidget = currentWindow->getTabWidget();

        QJsonObject winObject = winIt->toObject();
        QJsonArray tabArray = winObject.value("tabs").toArray();
        for (auto tabIt = tabArray.constBegin(); tabIt != tabArray.constEnd(); ++tabIt)
        {
            tabWidget->openLinkInNewTab(QUrl::fromUserInput(tabIt->toString()));
        }

        // Close unused first tab
        tabWidget->closeTab(0);

        // Set current tab to the last active tab
        int currentTab = winObject.value("current_tab").toInt();
        tabWidget->setCurrentIndex(currentTab);
    }
}
