#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include "FaviconStoreBridge.h"
#include "WebPage.h"

FaviconStoreBridge::FaviconStoreBridge(WebPage *parent) :
    QObject(parent),
    m_page(parent)
{
}

FaviconStoreBridge::~FaviconStoreBridge()
{
}

void FaviconStoreBridge::updateIconUrl(const QString &faviconUrl)
{
    if (faviconUrl.isEmpty())
        return;
    sBrowserApplication->getFaviconStorage()->updateIcon(faviconUrl, m_page->url(), m_page->icon());
}
