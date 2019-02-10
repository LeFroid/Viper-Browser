#include "BrowserApplication.h"
#include "FaviconStore.h"
#include "FaviconStoreBridge.h"
#include "WebPage.h"

FaviconStoreBridge::FaviconStoreBridge(FaviconStore *faviconStore, WebPage *parent) :
    QObject(parent),
    m_page(parent),
    m_faviconStore(faviconStore)
{
}

FaviconStoreBridge::~FaviconStoreBridge()
{
}

void FaviconStoreBridge::updateIconUrl(const QString &faviconUrl)
{
    if (faviconUrl.isEmpty())
        return;

    m_faviconStore->updateIcon(faviconUrl, m_page->url(), m_page->icon());
}
