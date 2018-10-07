#include "BrowserApplication.h"
#include "FaviconStore.h"
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

    QIcon icon = m_page->icon();
    if (icon.isNull())
        return;

    sBrowserApplication->getFaviconStore()->updateIcon(faviconUrl, m_page->url(), icon);
}
