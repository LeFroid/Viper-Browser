#include <algorithm>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QUrl>
#include <QWebEngineContextMenuData>
#include <QWebEngineProfile>
#include <QWheelEvent>

#include "BrowserApplication.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "SearchEngineManager.h"
#include "Settings.h"
#include "WebDialog.h"
#include "WebView.h"
#include "WebPage.h"

WebView::WebView(bool privateView, QWidget *parent) :
    QWebEngineView(parent),
    m_page(nullptr),
    m_progress(0),
    m_privateView(privateView),
    m_contextMenuHelper(),
    m_jsCallbackResult()
{
    //setAcceptDrops(true);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    if (privateView)
    {
        auto profile = sBrowserApplication->getPrivateBrowsingProfile();
        m_page = new WebPage(profile, this); 
    }
    else
        m_page = new WebPage(this);

    setPage(m_page);

    // Setup link hover signal
    connect(m_page, &WebPage::linkHovered, this, &WebView::linkHovered);

    connect(this, &WebView::loadProgress, [=](int value){
       m_progress = value;
    });

    // Load context menu helper script for image URL detection
    QFile contextScriptFile(":/ContextMenuHelper.js");
    if (contextScriptFile.open(QIODevice::ReadOnly))
    {
        m_contextMenuHelper = QString(contextScriptFile.readAll());
        contextScriptFile.close();
    }
}

int WebView::getProgress() const
{
    return m_progress;
}

void WebView::loadBlankPage()
{
    QFile resource(":/blank.html");
    bool opened = resource.open(QIODevice::ReadOnly);
    if (opened)
        setHtml(QString::fromUtf8(resource.readAll().constData()), QUrl("about:blank"));
}

bool WebView::isOnBlankPage() const
{
    return url() == QUrl("about:blank");
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

    int x = static_cast<int>(static_cast<qreal>(pos.x()) / m_page->zoomFactor());
    int y = static_cast<int>(static_cast<qreal>(pos.y()) / m_page->zoomFactor());
    scriptCopy.replace(QLatin1String("{{x}}"), QString::number(x));
    scriptCopy.replace(QLatin1String("{{y}}"), QString::number(y));

    return scriptCopy;
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    auto contextMenuData = m_page->contextMenuData();
    QMenu menu;

    // Link menu options
    const auto linkUrl = contextMenuData.linkUrl();
    if (!linkUrl.isEmpty())
    {
        menu.addAction(tr("Open link in new tab"), [=](){
            emit openInNewTabRequest(linkUrl);
        });
        menu.addAction(tr("Open link in new window"), [=](){
            emit openInNewWindowRequest(linkUrl, m_privateView);
        });
        menu.addSeparator();
    }

    auto mediaType = contextMenuData.mediaType();

    // Image menu options
    if (mediaType == QWebEngineContextMenuData::MediaTypeImage)
    {
        menu.addAction(tr("Open image in new tab"), [=](){
            emit openInNewTabRequest(contextMenuData.mediaUrl());
        });
        menu.addAction(tr("Open image"), [=](){
            emit openRequest(contextMenuData.mediaUrl());
        });
        menu.addSeparator();

        QAction *a = pageAction(WebPage::DownloadImageToDisk);
        a->setText(tr("Save image as..."));
        menu.addAction(a);

        menu.addAction(pageAction(WebPage::CopyImageToClipboard));
        menu.addAction(pageAction(WebPage::CopyImageUrlToClipboard));

        menu.addSeparator();
    }
    else if (mediaType == QWebEngineContextMenuData::MediaTypeVideo
             || mediaType == QWebEngineContextMenuData::MediaTypeAudio)
    {
        menu.addAction(pageAction(WebPage::ToggleMediaPlayPause));
        menu.addAction(pageAction(WebPage::ToggleMediaMute));
        menu.addAction(pageAction(WebPage::ToggleMediaLoop));
        menu.addAction(pageAction(WebPage::ToggleMediaControls));
        menu.addSeparator();

        menu.addAction(pageAction(WebPage::DownloadMediaToDisk));
        menu.addAction(pageAction(WebPage::CopyMediaUrlToClipboard));
        menu.addSeparator();
    }

    // Link menu options continued
    if (!linkUrl.isEmpty())
    {
        menu.addAction(pageAction(WebPage::DownloadLinkToDisk));
        menu.addAction(pageAction(WebPage::CopyLinkToClipboard));
        menu.addSeparator();
    }

    // Text selection options
    if (!contextMenuData.selectedText().isEmpty())
    {
        const QString text = contextMenuData.selectedText();

        const bool isEditable = contextMenuData.isContentEditable();
        if (isEditable)
            menu.addAction(pageAction(WebPage::Cut));
        menu.addAction(pageAction(WebPage::Copy));
        if (isEditable)
        {
            QClipboard *clipboard = sBrowserApplication->clipboard();
            if (!clipboard->text(QClipboard::Clipboard).isEmpty())
                menu.addAction(pageAction(WebPage::Paste));
        }

        // Search for current selection menu option
        SearchEngineManager *searchMgr = &SearchEngineManager::instance();
        menu.addAction(tr("Search %1 for selected text").arg(searchMgr->getDefaultSearchEngine()), [=](){
            QString searchUrl = searchMgr->getQueryString(searchMgr->getDefaultSearchEngine());
            searchUrl.replace("=%s", QString("=%1").arg(text));
            emit openInNewTabRequest(QUrl::fromUserInput(searchUrl));
        });

        QUrl selectionUrl = QUrl::fromUserInput(text);
        if (selectionUrl.isValid())
        {
            menu.addAction(tr("Go to %1").arg(text), [=](){
                emit openInNewTabRequest(selectionUrl, true);
            });
        }

        menu.addSeparator();
    }

    // Default menu options
    if (linkUrl.isEmpty()
            && contextMenuData.selectedText().isEmpty()
            && contextMenuData.mediaType() == QWebEngineContextMenuData::MediaTypeNone)
    {
        menu.addAction(pageAction(WebPage::Back));
        menu.addAction(pageAction(WebPage::Forward));
        menu.addAction(pageAction(WebPage::Reload));
        menu.addSeparator();
    }

    // Web inspector
    menu.addAction(tr("Inspect element"), this, &WebView::inspectElement);

    menu.exec(event->globalPos());
}

void WebView::wheelEvent(QWheelEvent *event)
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
    QWebEngineView::wheelEvent(event);
}

QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type)
{
    switch (type)
    {
        case QWebEnginePage::WebBrowserWindow:    // Open a new window
        {
            MainWindow *win = m_privateView ? sBrowserApplication->getNewPrivateWindow() : sBrowserApplication->getNewWindow();
            return win->getTabWidget()->currentWebView();
        }	
        case QWebEnginePage::WebBrowserTab:
        {
            // Get main window
            QObject *obj = parent();
            while (obj->parent() != nullptr)
                obj = obj->parent();

            if (MainWindow *mw = static_cast<MainWindow*>(obj))
            {
                return mw->getNewTabWebView();
            }
            break;
        }
        case QWebEnginePage::WebDialog:     // Open a web dialog
        {
            WebDialog *dialog = new WebDialog(m_privateView);
            dialog->show();
            return dialog->getView();
        }
        default: break;
    }
    return QWebEngineView::createWindow(type);
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
