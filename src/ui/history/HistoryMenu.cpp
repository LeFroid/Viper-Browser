#include "HistoryMenu.h"
#include "FaviconManager.h"
#include "HistoryManager.h"

#include <QAction>
#include <QList>
#include <QKeySequence>
#include <QDebug>
#include <QTimer>

HistoryMenu::HistoryMenu(QWidget *parent) :
    QMenu(parent),
    m_historyManager(nullptr),
    m_faviconManager(nullptr),
    m_actionShowHistory(nullptr),
    m_actionClearHistory(nullptr)
{
}

HistoryMenu::HistoryMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent),
    m_historyManager(nullptr),
    m_faviconManager(nullptr),
    m_actionShowHistory(nullptr),
    m_actionClearHistory(nullptr)
{
}

HistoryMenu::~HistoryMenu()
{
}

void HistoryMenu::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_historyManager = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");
    m_faviconManager = serviceLocator.getServiceAs<FaviconManager>("FaviconManager");

    setup();
}

void HistoryMenu::addHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon)
{
    QAction *historyItem = new QAction(title);
    historyItem->setIcon(favicon);
    connect(historyItem, &QAction::triggered, this, [this, url](){
        emit loadUrl(url);
    });

    addAction(historyItem);

    clearOldestEntries();
}

void HistoryMenu::prependHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon)
{
    QList<QAction*> menuActions = actions();

    QAction *beforeItem = nullptr;
    if (menuActions.size() > 3)
        beforeItem = menuActions[3];

    QAction *historyItem = new QAction(title);
    historyItem->setIcon(favicon);
    connect(historyItem, &QAction::triggered, this, [this, url](){
        emit loadUrl(url);
    });

    insertAction(beforeItem, historyItem);

    clearOldestEntries();
}

void HistoryMenu::clearItems()
{
    QList<QAction*> menuActions = actions();
    if (menuActions.size() < 3)
        return;

    for (int i = 3; i < menuActions.size(); ++i)
    {
        QAction *currItem = menuActions.at(i);
        removeAction(currItem);
        delete currItem;
    }
}

void HistoryMenu::resetItems()
{
    clearItems();

    const std::deque<HistoryEntry> &historyItems = m_historyManager->getRecentItems();
    for (auto it = historyItems.begin(); it != historyItems.end(); ++it)
    {
        if (it->Title.isEmpty())
            continue;

        if (m_faviconManager)
            addHistoryItem(it->URL, it->Title, m_faviconManager->getFavicon(it->URL));
    }
}

void HistoryMenu::onPageVisited(const QUrl &url, const QString &title)
{
    if (!m_faviconManager)
    {
        prependHistoryItem(url, title, QIcon());
        return;
    }

    prependHistoryItem(url, title, m_faviconManager->getFavicon(url));
}

void HistoryMenu::setup()
{
    connect(m_historyManager, &HistoryManager::pageVisited,    this, &HistoryMenu::onPageVisited);
    connect(m_historyManager, &HistoryManager::historyCleared, this, &HistoryMenu::resetItems, Qt::QueuedConnection);

    m_actionShowHistory = addAction(QLatin1String("&Show all History"));
    m_actionShowHistory->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_H));

    m_actionClearHistory = addAction(QLatin1String("&Clear Recent History..."));
    m_actionClearHistory->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Delete));

    addSeparator();

    QTimer::singleShot(500, this, &HistoryMenu::resetItems);//resetItems();
}

void HistoryMenu::clearOldestEntries()
{
    QList<QAction*> menuActions = actions();

    int menuSize = menuActions.size();
    while (menuSize > 17)
    {
        QAction *actionToRemove = menuActions[menuSize - 1];
        removeAction(actionToRemove);
        delete actionToRemove;
        --menuSize;
    }
}
