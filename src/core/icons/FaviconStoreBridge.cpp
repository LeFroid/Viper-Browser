#include "BrowserApplication.h"
#include "FaviconManager.h"
#include "FaviconStoreBridge.h"
#include "WebPage.h"
#include <QUrl>

FaviconStoreBridge::FaviconStoreBridge(FaviconManager *faviconManager, WebPage *parent) :
    QObject(parent),
    m_page(parent),
    m_faviconManager(faviconManager)
{
}

FaviconStoreBridge::~FaviconStoreBridge()
{
}

void FaviconStoreBridge::updateIconUrl(const QString &faviconUrl)
{
    if (faviconUrl.isEmpty())
        return;

    m_faviconManager->updateIcon(QUrl(faviconUrl), m_page->url(), m_page->icon());
}
