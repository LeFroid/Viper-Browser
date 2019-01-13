#include "BrowserApplication.h"
#include "FavoritePagesManager.h"
#include "HistoryManager.h"

FavoritePagesManager::FavoritePagesManager(HistoryManager *historyMgr, QObject *parent) :
    QObject(parent),
    m_timerId(0),
    m_mostVisitedPages(),
    m_historyManager(historyMgr)
{
    // update page list every 10 minutes
    m_timerId = startTimer(1000 * 60 * 10);
    loadFavorites();
}

FavoritePagesManager::~FavoritePagesManager()
{
    killTimer(m_timerId);
}

void FavoritePagesManager::timerEvent(QTimerEvent */*event*/)
{
    loadFavorites();
}

void FavoritePagesManager::loadFavorites()
{
    if (!m_historyManager)
        return;

    m_mostVisitedPages = m_historyManager->loadMostVisitedEntries(10);
    //todo: iterate through the vector, calling
    //        page.Thumbnail = thumbnailStore->getThumbnail(page.URL)
    //      for each item
}
