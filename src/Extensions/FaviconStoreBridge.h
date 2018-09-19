#ifndef FAVICONSTOREBRIDGE_H
#define FAVICONSTOREBRIDGE_H

#include <QObject>

class WebPage;

/**
 * @class FaviconStoreBridge
 * @brief Bridge between the favicon url-searching script injected into each \ref WebPage ,
 *        and the \ref FaviconStorage system
 */
class FaviconStoreBridge : public QObject
{
    Q_OBJECT

public:
    /// Constructs the favicon store bridge, given a pointer to the parent web page
    FaviconStoreBridge(WebPage *parent);

    /// Destructor
    ~FaviconStoreBridge();

public slots:
    /// Called when the parent web page has finished loading and has located the URL of its favicon
    void updateIconUrl(const QString &faviconUrl);

private:
    /// Pointer to the page that owns this bridge
    WebPage *m_page;
};

#endif // FAVICONSTOREBRIDGE_H
