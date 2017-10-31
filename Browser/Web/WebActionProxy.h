#ifndef WEBACTIONPROXY_H
#define WEBACTIONPROXY_H

#include <QObject>
#include <QWebPage>

class QAction;

/**
 * @class WebActionProxy
 * @brief Acts as a proxy for a WebAction on the current page
 */
class WebActionProxy : public QObject
{
    Q_OBJECT
public:
    /// Constructs a new web action proxy
    explicit WebActionProxy(QWebPage::WebAction action, QAction *proxyAction, QObject *parent = nullptr);

    /// Sets the pointer to the active web page
    void setPage(QWebPage *page);

private slots:
    /// Called to trigger the web page's WebAction when the main UI's associated
    /// action is triggered
    void triggerOnPage();

    /// Called when the web page's action has been changed
    void onChange();

    /// Called in the event that the web page has been destroyed
    void onPageDestroyed();

    /// Called in the event that the proxy action has been destroyed
    void onProxyDestroyed();

private:
    /// Proxy action
    QAction *m_proxyAction;

    /// Web page associated with the action proxy
    QWebPage *m_page;

    /// Web page action type
    QWebPage::WebAction m_pageAction;
};

#endif // WEBACTIONPROXY_H
