#include "WebWidget.h"
#include "WebView.h"

#include <QIcon>
#include <QUrl>

WebWidget::WebWidget(bool privateMode, QWidget *parent) :
    QWidget(parent)
{
    m_view = new WebView(privateMode, this);

    connect(m_view, &WebView::iconChanged, this, &WebWidget::iconChanged);
    connect(m_view, &WebView::iconUrlChanged, this, &WebWidget::iconUrlChanged);
}

int WebWidget::getProgress() const
{
    return m_view->getProgress();
}

void WebWidget::loadBlankPage()
{
    m_view->loadBlankPage();
}

bool WebWidget::isOnBlankPage() const
{
    return m_view->isOnBlankPage();
}

QIcon WebWidget::getIcon() const
{
    return m_view->icon();
}

QUrl WebWidget::getIconUrl() const
{
    return m_view->iconUrl();
}

QString WebWidget::getTitle() const
{
    return m_view->getTitle();
}

void WebWidget::load(const QUrl &url)
{
    m_view->load(url);
}
