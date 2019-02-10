#ifndef FAVICONSTOREBRIDGE_H
#define FAVICONSTOREBRIDGE_H

#include <QObject>

class FaviconStore;
class WebPage;

/**
 * @class FaviconStoreBridge
 * @brief Bridge between the favicon url-searching script injected into each \ref WebPage ,
 *        and the \ref FaviconStore system
 */
class FaviconStoreBridge : public QObject
{
    Q_OBJECT

public:
    /// Constructs the favicon store bridge, given a pointer to the parent web page
    explicit FaviconStoreBridge(FaviconStore *faviconStore, WebPage *parent);

    /// Destructor
    ~FaviconStoreBridge();

public slots:
    /// Called when the parent web page has finished loading and has located the URL of its favicon
    void updateIconUrl(const QString &faviconUrl);

private:
    /// Pointer to the page that owns this bridge
    WebPage *m_page;

    /// Pointer to the favicon store
    FaviconStore *m_faviconStore;
};

#endif // FAVICONSTOREBRIDGE_H
