#include <algorithm>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QUrl>
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
    setAcceptDrops(true);

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
        setHtml(QString::fromUtf8(resource.readAll().constData()));
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
    // Add menu items missing from default context menu
    QMenu *menu = m_page->createStandardContextMenu();
    const auto actions = menu->actions();
    auto it = std::find(actions.cbegin(), actions.cend(), m_page->action(QWebEnginePage::OpenLinkInThisWindow));
    if (it != actions.end())
    {
        (*it)->setText(tr("Open link"));
        ++it;

        QAction *before = (it == actions.cend() ? nullptr: *it);
        menu->insertAction(before, m_page->action(QWebEnginePage::OpenLinkInNewTab));
        menu->insertAction(before, m_page->action(QWebEnginePage::OpenLinkInNewWindow));
    }
    it = std::find(actions.cbegin(), actions.cend(), m_page->action(QWebEnginePage::CopyImageUrlToClipboard));
    if (it != actions.end())
    {
        QString contextHelperScript = getContextMenuScript(event->pos());
        m_page->runJavaScriptNonBlocking(contextHelperScript, m_jsCallbackResult);

        auto actions = menu->actions();

        QAction *openImageNewTabAction = new QAction(tr("Open image in a new tab"), menu);
        connect(openImageNewTabAction, &QAction::triggered, [=](bool){
            QUrl imageUrl = QUrl(m_jsCallbackResult.toString());
            emit openInNewTabRequest(imageUrl);
        });

        QAction *openImageThisTabAction = new QAction(tr("Open image in this tab"), menu);
        connect(openImageThisTabAction, &QAction::triggered, [=](bool){
            QUrl imageUrl = QUrl(m_jsCallbackResult.toString());
            emit openRequest(imageUrl);
        });

        menu->insertAction(actions.at(0), openImageNewTabAction);
        menu->insertAction(actions.at(0), openImageThisTabAction);
        menu->insertSeparator(actions.at(0));
    }
    it = std::find(actions.cbegin(), actions.cend(), m_page->action(QWebEnginePage::InspectElement));
    if (it == actions.end())
    {
        menu->addSeparator();
        menu->addAction(tr("Inspect element"), this, &WebView::inspectElement);
    }
    else
    {
        menu->addAction(tr("Open inspector"), this, &WebView::inspectElement);
        menu->removeAction((*it));
        menu->insertAction(nullptr, m_page->action(QWebEnginePage::InspectElement));
    }
    menu->popup(event->globalPos());
/*

    // Check if context menu includes links
    if (!res.linkUrl().isEmpty())
    {
        menu.addAction(tr("Open link in new tab"), [=](){
            emit openInNewTabRequest(res.linkUrl());
        });
        menu.addAction(tr("Open link in new window"), [=](){
            emit openInNewWindowRequest(res.linkUrl(), this->settings()->testAttribute(QWebSettings::PrivateBrowsingEnabled));
        });
        menu.addSeparator();
        menu.addAction(pageAction(QWebPage::DownloadLinkToDisk));
        menu.addAction(pageAction(QWebPage::CopyLinkToClipboard));
        addInspectorIfEnabled(&menu);
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
    // Check if context menu is activated when mouse is on an image
    if (!res.imageUrl().isEmpty())
    {
        menu.addAction(tr("Open image"), [=](){
            emit openRequest(res.imageUrl());
        });
        menu.addAction(tr("Open image in new tab"), [=](){
            emit openInNewTabRequest(res.imageUrl());
        });
        menu.addSeparator();
        menu.addAction(tr("Save image as..."), [=](){
            pageAction(QWebPage::DownloadImageToDisk)->trigger();
        });
        menu.addAction(tr("Copy image"), [=]() { pageAction(QWebPage::CopyImageToClipboard)->trigger(); });
        menu.addAction(tr("Copy image address"), [=]() { pageAction(QWebPage::CopyImageUrlToClipboard)->trigger(); });
        addInspectorIfEnabled(&menu);
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
    // Check if context menu is activated when there is a text selection
    if (!page()->selectedText().isEmpty())
    {
        // Text selection menu
        menu.addAction(pageAction(QWebPage::Copy));

        // Search for current selection menu option
        SearchEngineManager *searchMgr = &SearchEngineManager::instance();
        menu.addAction(tr("Search %1 for selected text").arg(searchMgr->getDefaultSearchEngine()), [=](){
            QString searchUrl = searchMgr->getQueryString(searchMgr->getDefaultSearchEngine());
            searchUrl.replace("=%s", QString("=%1").arg(page()->selectedText()));
            emit openInNewTabRequest(QUrl::fromUserInput(searchUrl));
        });

        // Check if selection could be a URL
        QString selection = page()->selectedText();
        QUrl selectionUrl = QUrl::fromUserInput(selection);
        if (selectionUrl.isValid())
        {
            menu.addAction(tr("Go to %1").arg(selection), [=](){
                emit openInNewTabRequest(selectionUrl, true);
            });
        }

        addInspectorIfEnabled(&menu);
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
    // Default menu
    menu.addAction(pageAction(QWebPage::Back));
    menu.addAction(pageAction(QWebPage::Forward));
    menu.addAction(pageAction(QWebPage::Reload));
    addInspectorIfEnabled(&menu);
    menu.exec(mapToGlobal(event->pos()));
	    */
}

void WebView::wheelEvent(QWheelEvent *event)
{
    QUrl emptyUrl;
    emit linkHovered(emptyUrl);
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
