#include "WebActionProxy.h"
#include <QAction>

WebActionProxy::WebActionProxy(WebPage::WebAction action, QAction *proxyAction, QObject *parent) :
    QObject(parent),
    m_proxyAction(proxyAction),
    m_page(nullptr),
    m_pageAction(action)
{
    // Disable action by default
    m_proxyAction->setEnabled(false);

    // Connect signals to slots
    connect(m_proxyAction, &QAction::triggered, this, &WebActionProxy::triggerOnPage);
    connect(m_proxyAction, &QAction::destroyed, this, &WebActionProxy::onProxyDestroyed);
}

void WebActionProxy::setPage(WebPage *page)
{
    if (!page || !m_proxyAction)
        return;

    if (m_page)
        disconnect(m_page, &WebPage::destroyed, this, &WebActionProxy::onPageDestroyed);

    m_page = page;

    // Check if proxy action should be enabled
    QAction *action = m_page->action(m_pageAction);
    m_proxyAction->setEnabled(action->isEnabled());

    connect(action, &QAction::changed, this, &WebActionProxy::onChange);
    connect(m_page, &WebPage::destroyed, this, &WebActionProxy::onPageDestroyed);
}

void WebActionProxy::triggerOnPage()
{
    if (m_page)
        m_page->action(m_pageAction)->trigger();
}

void WebActionProxy::onChange()
{
    if (QAction *source = qobject_cast<QAction*>(sender()))
    {
        if (m_proxyAction && m_page && source->parent() == m_page)
        {
            m_proxyAction->setEnabled(source->isEnabled());
        }
    }
}

void WebActionProxy::onPageDestroyed()
{
    m_page = nullptr;
}

void WebActionProxy::onProxyDestroyed()
{
    m_proxyAction = nullptr;
}
