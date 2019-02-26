#include "HistoryMenu.h"
#include "FaviconStore.h"
#include "HistoryManager.h"

#include <QAction>
#include <QList>
#include <QKeySequence>
#include <QDebug>

HistoryMenu::HistoryMenu(QWidget *parent) :
    QMenu(parent),
    m_actionShowHistory(nullptr),
    m_actionClearHistory(nullptr)
{
}

HistoryMenu::HistoryMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent)
{
}

HistoryMenu::~HistoryMenu()
{
}

void HistoryMenu::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_historyManager = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");
    m_faviconStore   = serviceLocator.getServiceAs<FaviconStore>("FaviconStore");

    setup();
}

void HistoryMenu::addHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon)
{
    QAction *historyItem = new QAction(title);
    historyItem->setIcon(favicon);
    connect(historyItem, &QAction::triggered, [=](){
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
    connect(historyItem, &QAction::triggered, [=](){
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
    for (auto &it : historyItems)
    {
        if (!it.Title.isEmpty())
            addHistoryItem(it.URL, it.Title, m_faviconStore ? m_faviconStore->getFavicon(it.URL) : QIcon());
    }
}

void HistoryMenu::onPageVisited(const QUrl &url, const QString &title)
{
    QIcon favicon = m_faviconStore ? m_faviconStore->getFavicon(url) : QIcon();
    prependHistoryItem(url, title, favicon);
}

void HistoryMenu::setup()
{
    connect(m_historyManager, &HistoryManager::pageVisited, this, &HistoryMenu::onPageVisited);
    connect(m_historyManager, &HistoryManager::historyCleared, this, &HistoryMenu::resetItems);

    m_actionShowHistory = addAction(QLatin1String("&Show all History"));
    m_actionShowHistory->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_H));

    m_actionClearHistory = addAction(QLatin1String("&Clear Recent History..."));
    m_actionClearHistory->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Delete));

    addSeparator();

    resetItems();
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
