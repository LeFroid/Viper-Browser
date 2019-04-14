#include "SessionManager.h"
#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "CommonUtil.h"
#include "MainWindow.h"
#include "WebHistory.h"
#include "WebWidget.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
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
            WebWidget *ww = tabWidget->getWebWidget(i);
            if (!ww)
                continue;

            QJsonObject tabInfoObj;
            tabInfoObj.insert(QLatin1String("url"), QJsonValue(ww->url().toString()));
            tabInfoObj.insert(QLatin1String("is_pinned"), QJsonValue(tabWidget->isTabPinned(i)));
            tabInfoObj.insert(QLatin1String("is_hibernating"), QJsonValue(ww->isHibernating()));
            tabInfoObj.insert(QLatin1String("title"), QJsonValue(ww->getTitle()));

            QIcon icon = ww->getIcon();
            if (icon.isNull())
                icon = tabWidget->tabIcon(i);

            QByteArray faviconB64 = CommonUtil::iconToBase64(icon);
            auto faviconEncoded = QLatin1String(faviconB64.data());
            tabInfoObj.insert(QLatin1String("icon"), QJsonValue(faviconEncoded));
            tabInfoObj.insert(QLatin1String("icon_url"), QJsonValue(ww->getIconUrl().toString()));

            QByteArray tabHistoryB64 = ww->getEncodedHistory().toBase64();
            auto tabHistoryEncoded = QLatin1String(tabHistoryB64.data());
            tabInfoObj.insert(QLatin1String("history"), QJsonValue(tabHistoryEncoded));

            tabArray.append(QJsonValue(tabInfoObj));
        }

        winObj.insert(QLatin1String("tabs"), QJsonValue(tabArray));
        winObj.insert(QLatin1String("current_tab"), QJsonValue(tabWidget->currentIndex()));

        // Store geometry of window
        const QRect &winGeom = win->geometry();
        winObj.insert(QLatin1String("geom_x"), QJsonValue(winGeom.x()));
        winObj.insert(QLatin1String("geom_y"), QJsonValue(winGeom.y()));
        winObj.insert(QLatin1String("geom_width"), QJsonValue(winGeom.width()));
        winObj.insert(QLatin1String("geom_height"), QJsonValue(winGeom.height()));

        winObj.insert(QLatin1String("is_maximized"), QJsonValue(win->isMaximized()));

        windowArray.append(QJsonValue(winObj));
    }
    sessionObj.insert(QLatin1String("windows"), QJsonValue(windowArray));

    // Save to file
    QJsonDocument sessionDoc(sessionObj);
    dataFile.write(sessionDoc.toJson());
    dataFile.close();

    m_savedSession = true;
}

void SessionManager::restoreSession(MainWindow *firstWindow, BrowserApplication *browserApplication)
{
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
    auto it = sessionObj.find(QLatin1String("windows"));
    if (it == sessionObj.end())
        return;

    // Load each tab into the appropriate windows
    bool isFirstWindow = true;
    MainWindow *currentWindow = firstWindow;

    QJsonArray winArray = it.value().toArray();
    for (auto winIt = winArray.constBegin(); winIt != winArray.constEnd(); ++winIt)
    {
        if (!isFirstWindow)
            currentWindow = browserApplication->getNewWindow();

        isFirstWindow = false;

        QJsonObject winObject = winIt->toObject();

        // Restore window properties
        const bool isMaximized = winObject.value(QLatin1String("is_maximized")).toBool(false);

        if (isMaximized)
            currentWindow->showMaximized();
        else if (winObject.contains(QLatin1String("geom_x")))
        {
            QRect winGeom;
            winGeom.setX(winObject.value(QLatin1String("geom_x")).toInt());
            winGeom.setY(winObject.value(QLatin1String("geom_y")).toInt());
            winGeom.setWidth(winObject.value(QLatin1String("geom_width")).toInt(100));
            winGeom.setHeight(winObject.value(QLatin1String("geom_height")).toInt(100));
            currentWindow->setGeometry(winGeom);
        }

        // Restore tabs belonging to the window
        BrowserTabWidget *tabWidget = currentWindow->getTabWidget();

        QJsonArray tabArray = winObject.value(QLatin1String("tabs")).toArray();
        int i = 0;
        for (auto tabIt = tabArray.constBegin(); tabIt != tabArray.constEnd(); ++tabIt)
        {
            if (tabIt->isString())
                tabWidget->openLinkInNewTab(QUrl::fromUserInput(tabIt->toString()));
            else if (tabIt->isObject())
            {
                QJsonObject tabInfoObj = tabIt->toObject();

                WebState webState;
                webState.title = tabInfoObj.value(QLatin1String("title")).toString();
                webState.iconUrl = QUrl::fromUserInput(tabInfoObj.value(QLatin1String("icon_url")).toString());
                webState.url = QUrl::fromUserInput(tabInfoObj.value(QLatin1String("url")).toString());

                const QByteArray faviconData = tabInfoObj.value(QLatin1String("icon")).toString().toLatin1();
                if (!faviconData.isNull())
                    webState.icon = CommonUtil::iconFromBase64(faviconData);

                const QByteArray encodedHistory = tabInfoObj.value(QLatin1String("history")).toString().toLatin1();
                webState.pageHistory = QByteArray::fromBase64(encodedHistory);

                WebWidget *ww = (i == 0) ? qobject_cast<WebWidget*>(tabWidget->widget(0)) : tabWidget->newBackgroundTabAtIndex(i);

                tabWidget->setTabPinned(i,
                                        tabInfoObj.value(QLatin1String("is_pinned")).toBool());

                ww->setHibernation(tabInfoObj.value(QLatin1String("is_hibernating")).toBool());

                ww->setWebState(std::move(webState));

                ++i;
            }
        }

        // Set current tab to the last active tab
        int currentTab = winObject.value(QLatin1String("current_tab")).toInt();
        tabWidget->setCurrentIndex(currentTab);
    }
}
