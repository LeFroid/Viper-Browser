#include <algorithm>
#include <string>

#include <QClipboard>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFontMetrics>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QUrl>
#include <QTimer>
#include <QWebEngineContextMenuData>
#include <QWebEngineHttpRequest>
#include <QWebEngineProfile>
#include <QWheelEvent>
#include <QQuickWidget>

#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "HttpRequest.h"
#include "MainWindow.h"
#include "SearchEngineManager.h"
#include "Settings.h"
#include "WebDialog.h"
#include "WebHitTestResult.h"
#include "WebView.h"
#include "WebPage.h"
#include "WebWidget.h"

WebView::WebView(bool privateView, QWidget *parent) :
    QWebEngineView(parent),
    m_page(nullptr),
    m_settings(nullptr),
    m_progress(0),
    m_privateView(privateView),
    m_contextMenuHelper(),
    m_jsCallbackResult(),
    m_viewFocusProxy(nullptr),
    m_thumbnail()
{
    setAcceptDrops(true);
    setObjectName(QLatin1String("webView"));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

    // Load context menu helper script for image URL detection
    QFile contextScriptFile(QLatin1String(":/ContextMenuHelper.js"));
    if (contextScriptFile.open(QIODevice::ReadOnly))
    {
        m_contextMenuHelper = QString(contextScriptFile.readAll());
        contextScriptFile.close();
    }
}

WebView::~WebView()
{
}

int WebView::getProgress() const
{
    return m_progress;
}

void WebView::loadBlankPage()
{
    load(QUrl(QLatin1String("viper://blank")));
}

bool WebView::isOnBlankPage() const
{
    const QUrl currentUrl = url();
    return (currentUrl == QUrl(QLatin1String("viper://blank"))
            || currentUrl == QUrl(QLatin1String("viper://newtab")));
}

QString WebView::getTitle() const
{
    QString pageTitle = title();

    if (!pageTitle.isEmpty())
        return pageTitle;

    QUrl pageUrl = url();

    // Try to extract end of path
    pageTitle = pageUrl.path();
    if (pageTitle.size() > 1)
    {
        if (pageTitle.contains(QLatin1Char('/')))
            return pageTitle.mid(pageTitle.lastIndexOf(QLatin1Char('/')) + 1);

        return pageTitle;
    }

    // Default to host
    return pageUrl.host();
}

const QPixmap &WebView::getThumbnail()
{
    if (m_thumbnail.isNull())
        makeThumbnailOfPage();
    return m_thumbnail;
}

void WebView::load(const QUrl &url)
{
    if (!m_page->acceptNavigationRequest(url, WebPage::NavigationTypeTyped, true))
        return;

    m_progress = 0;
    setUrl(url);
    //QWebEngineView::load(url);
}

void WebView::load(const HttpRequest &request)
{
    if (!m_page->acceptNavigationRequest(request.getUrl(), WebPage::NavigationTypeTyped, true))
        return;

    m_progress = 0;
    m_page->load(request.toWebEngineRequest());
}

QWidget *WebView::getViewFocusProxy()
{
    if (!m_viewFocusProxy.isNull())
        return m_viewFocusProxy;

    if (focusProxy() != nullptr)
        return focusProxy();

    QList<QQuickWidget*> children = findChildren<QQuickWidget*>();
    for (int i = children.size() - 1; i >= 0; --i)
    {
        QQuickWidget *w = children.at(i);
        if (w && w->isVisible())
            return w;
    }
    return this;
}

WebPage *WebView::getPage() const
{
    return m_page;
}

void WebView::setupPage(const ViperServiceLocator &serviceLocator)
{
    if (m_page != nullptr)
        return;

    const std::string profileName = m_privateView ? "PrivateWebProfile" : "PublicWebProfile";
    QWebEngineProfile *profile = serviceLocator.getServiceAs<QWebEngineProfile>(profileName);

    m_settings = serviceLocator.getServiceAs<Settings>("Settings");

    m_page = new WebPage(serviceLocator, profile);
    m_page->setParent(this);

    QWebEngineView::setPage(m_page);

    // Load start / progress / finish handlers
    connect(m_page, &WebPage::loadStarted, [this](){
        emit iconChanged(icon());
    });
    connect(m_page, &WebPage::loadProgress, [this](int value){
       m_progress = value;
    });
    connect(m_page, &WebPage::loadFinished, this, &WebView::onLoadFinished);

    // Handle full screen requests
    connect(m_page, &WebPage::fullScreenRequested, this, &WebView::onFullScreenRequested);
}

void WebView::resetZoom()
{
    setZoomFactor(1.0);
}

void WebView::zoomIn()
{
    setZoomFactor(zoomFactor() + 0.1);
}

void WebView::zoomOut()
{
    qreal currentZoom = zoomFactor();
    setZoomFactor(std::max(currentZoom - 0.1, 0.1));
}

QString WebView::getContextMenuScript(const QPoint &pos)
{
    QString scriptCopy = m_contextMenuHelper;
    QPointF posF(pos);
    return scriptCopy.arg(QString::number(posF.x() / zoomFactor())).arg(QString::number(posF.y() / zoomFactor()));
}

void WebView::showContextMenu(const QPoint &globalPos, const QPoint &relativePos)
{
    WebHitTestResult contextMenuData(m_page, getContextMenuScript(relativePos));

    const bool askWhereToSave = m_settings->getValue(BrowserSetting::AskWhereToSaveDownloads).toBool();

    QMenu *menu = new QMenu(this);

    // Link menu options
    const auto linkUrl = contextMenuData.linkUrl();
    if (!linkUrl.isEmpty())
    {
        menu->addAction(tr("Open link in new tab"), this, [=](){
            emit openInNewBackgroundTab(linkUrl);
        });
        menu->addAction(tr("Open link in new window"), this, [=](){
            emit openInNewWindowRequest(linkUrl, m_privateView);
        });
        menu->addSeparator();
    }

    auto mediaType = contextMenuData.mediaType();

    // Image menu options
    if (mediaType == WebHitTestResult::MediaTypeImage)
    {
        menu->addAction(tr("Open image in new tab"), this, [=](){
            emit openInNewBackgroundTab(contextMenuData.mediaUrl());
        });
        menu->addAction(tr("Open image"), this, [=](){
            emit openRequest(contextMenuData.mediaUrl());
        });
        menu->addSeparator();

        QAction *a = pageAction(WebPage::DownloadImageToDisk);

        // Set text to 'save image as...' if setting is to always ask where to save
        if (askWhereToSave)
            a->setText(tr("Save image as..."));
        menu->addAction(a);

        menu->addAction(pageAction(WebPage::CopyImageToClipboard));
        menu->addAction(pageAction(WebPage::CopyImageUrlToClipboard));

        menu->addSeparator();
    }
    else if (mediaType == WebHitTestResult::MediaTypeVideo
             || mediaType == WebHitTestResult::MediaTypeAudio)
    {
        menu->addAction(pageAction(WebPage::ToggleMediaPlayPause));
        menu->addAction(pageAction(WebPage::ToggleMediaMute));
        menu->addAction(pageAction(WebPage::ToggleMediaLoop));
        menu->addAction(pageAction(WebPage::ToggleMediaControls));
        menu->addSeparator();

        menu->addAction(pageAction(WebPage::DownloadMediaToDisk));
        menu->addAction(pageAction(WebPage::CopyMediaUrlToClipboard));
        menu->addSeparator();
    }

    // Link menu options continued
    if (!linkUrl.isEmpty())
    {
        QAction *a = pageAction(WebPage::DownloadLinkToDisk);
        if (askWhereToSave)
            a->setText(tr("Save link as..."));
        menu->addAction(a);

        menu->addAction(pageAction(WebPage::CopyLinkToClipboard));
        menu->addSeparator();
    }

    // Text selection options
    if (!contextMenuData.selectedText().isEmpty())
    {
        const QString text = contextMenuData.selectedText();

        const bool isEditable = contextMenuData.isContentEditable();
        if (isEditable)
            menu->addAction(pageAction(WebPage::Cut));
        menu->addAction(pageAction(WebPage::Copy));
        if (isEditable)
        {
            QClipboard *clipboard = sBrowserApplication->clipboard();
            if (!clipboard->text(QClipboard::Clipboard).isEmpty())
                menu->addAction(pageAction(WebPage::Paste));
        }

        // Search for current selection menu option
        QFontMetrics fontMetrics(font());
        const QString selectedTextInMenu = fontMetrics.elidedText(text, Qt::ElideRight, fontMetrics.width(QChar('R')) * 16);
        SearchEngineManager *searchMgr = &SearchEngineManager::instance();
        menu->addAction(tr("Search %1 for \"%2\"").arg(searchMgr->getDefaultSearchEngine()).arg(selectedTextInMenu), [=](){
            HttpRequest request = searchMgr->getSearchRequest(text);
            emit openHttpRequestInBackgroundTab(request);
        });

        QUrl selectionUrl = QUrl::fromUserInput(text);
        if (selectionUrl.isValid() && !selectionUrl.topLevelDomain().isEmpty())
        {
            menu->addAction(tr("Go to %1").arg(text), this, [=](){
                emit openInNewTab(selectionUrl);
            });
        }

        menu->addSeparator();
    }

    // Default menu options
    if (linkUrl.isEmpty()
            && contextMenuData.selectedText().isEmpty()
            && contextMenuData.mediaType() == WebHitTestResult::MediaTypeNone)
    {
        menu->addAction(pageAction(WebPage::Back));
        menu->addAction(pageAction(WebPage::Forward));
        menu->addAction(pageAction(WebPage::Reload));
        menu->addSeparator();
    }

    // Web inspector
    menu->addAction(tr("Inspect element"), this, &WebView::inspectElement);

    connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);

    menu->exec(globalPos);
}

void WebView::contextMenuEvent(QContextMenuEvent * /*event*/)
{
}

void WebView::onFullScreenRequested(QWebEngineFullScreenRequest request)
{
    emit fullScreenRequested(request.toggleOn());
    request.accept();
}

void WebView::onLoadFinished(bool ok)
{
    m_progress = 100;

    emit iconChanged(icon());

    if (ok && !isOnBlankPage())
        makeThumbnailOfPage();
    else
        m_thumbnail = QPixmap();
}

void WebView::makeThumbnailOfPage()
{
    QQuickWidget *qQuickChild = qobject_cast<QQuickWidget*>(focusProxy());
    if (qQuickChild)
    {
        m_thumbnail = qQuickChild->grab().scaled(QSize(375, 500));
        return;
    }

    QList<QQuickWidget*> children = findChildren<QQuickWidget*>();
    for (int i = children.size() - 1; i >= 0; --i)
    {
        QQuickWidget *w = children.at(i);
        if (w && w->isVisible())
        {
            qQuickChild = w;
            break;
        }
    }

    m_thumbnail = (qQuickChild != nullptr) ? qQuickChild->grab().scaled(QSize(375, 500)) : QPixmap();
}

void WebView::setViewFocusProxy(QWidget *w)
{
    m_viewFocusProxy = w;
}

void WebView::_mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button())
    {
        case Qt::LeftButton:
        case Qt::MiddleButton:
        {
            WebHitTestResult hitTest(m_page, getContextMenuScript(event->pos()));

            const QUrl linkUrl = hitTest.linkUrl();
            if (!linkUrl.isEmpty() && linkUrl.isValid())
            {
                if (event->button() == Qt::MiddleButton)
                {
                    emit openInNewBackgroundTab(linkUrl);
                    event->accept();
                }
                else if (event->modifiers() & Qt::ControlModifier)
                {
                     emit openInNewBackgroundTab(linkUrl);
                    event->accept();
                }
                else if (event->modifiers() & Qt::ShiftModifier)
                {
                    emit openInNewWindowRequest(linkUrl, m_privateView);
                    event->accept();
                }
            }

            break;
        }
        default:
            break;
    }
}

void WebView::_wheelEvent(QWheelEvent *event)
{
    auto keyModifiers = QApplication::keyboardModifiers();
    if (keyModifiers & Qt::ControlModifier)
    {
        QPoint angleDelta = event->angleDelta();
        if (angleDelta.y() > 0)
        {
            // Wheel rotated 'up' or away from user, trigger zoom in event
            zoomIn();
            event->accept();
            return;
        }
        else if (angleDelta.y() < 0)
        {
            // Wheel rotated 'down' or towards user, trigger zoom out event
            zoomOut();
            event->accept();
            return;
        }
    }

    //QWebEngineView::wheelEvent(event);
}

void WebView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-browser-tab"))
    {
        event->acceptProposedAction();
        return;
    }

    QWebEngineView::dragEnterEvent(event);
}

void WebView::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-browser-tab"))
    {
        qobject_cast<MainWindow*>(window())->dropEvent(event);
        return;
    }

    QWebEngineView::dropEvent(event);
}

void WebView::resizeEvent(QResizeEvent * /*event*/)
{
    if (QWidget *viewFocusProxy = getViewFocusProxy())
        viewFocusProxy->setGeometry(rect());
}
