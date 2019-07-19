#ifndef FAVICONSTOREBRIDGE_H
#define FAVICONSTOREBRIDGE_H

#include <QObject>

class FaviconManager;
class WebPage;

/**
 * @class FaviconStoreBridge
 * @brief Bridge between the favicon url-searching script injected into each \ref WebPage ,
 *        and the favicon management system
 */
class FaviconStoreBridge : public QObject
{
    Q_OBJECT

public:
    /// Constructs the favicon bridge, given a pointer to the favicon manager and the parent web page
    explicit FaviconStoreBridge(FaviconManager *faviconManager, WebPage *parent);

    /// Destructor
    ~FaviconStoreBridge();

public Q_SLOTS:
    /// Called when the parent web page has finished loading and has located the URL of its favicon
    void updateIconUrl(const QString &faviconUrl);

private:
    /// Pointer to the page that owns this bridge
    WebPage *m_page;

    /// Pointer to the favicon manager
    FaviconManager *m_faviconManager;
};

#endif // FAVICONSTOREBRIDGE_H
