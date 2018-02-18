#include <algorithm>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QUrl>
#include <QWebHitTestResult>
#include <QWheelEvent>

#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "MainWindow.h"
#include "SearchEngineManager.h"
#include "Settings.h"
#include "WebDialog.h"
#include "WebView.h"
#include "WebLinkLabel.h"
#include "WebPage.h"

WebView::WebView(QWidget *parent) :
    QWebView(parent),
    m_page(new WebPage(this)),
    m_progress(0)
{
    setAcceptDrops(true);
    setPage(m_page);

    // Setup link hover label
    m_labelLinkRef = new WebLinkLabel(this, m_page);

    connect(this, &WebView::loadProgress, [=](int value){
       m_progress = value;
    });

    // Connect QWebPage signals to slots in the web view
    connect(m_page, &WebPage::downloadRequested, this, &WebView::requestDownload);
}

void WebView::setPrivate(bool value)
{
    settings()->setAttribute(QWebSettings::PrivateBrowsingEnabled, value);
    if (value)
        m_page->enablePrivateBrowsing(); // handles private browsing mode cookies
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

void WebView::requestDownload(const QNetworkRequest &request)
{
    BrowserApplication *app = sBrowserApplication;
    bool askWhereToSave = app->getSettings()->getValue("AskWhereToSaveDownloads").toBool();
    app->getDownloadManager()->download(request, askWhereToSave);
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    QWebHitTestResult res = page()->mainFrame()->hitTestContent(event->pos());

    QMenu menu(this);

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
}

void WebView::resizeEvent(QResizeEvent *event)
{
    // move label
    QPoint labelPos(0, std::max(parentWidget()->geometry().height() - 17, 0));
    m_labelLinkRef->move(labelPos);

    QWebView::resizeEvent(event);
}

void WebView::wheelEvent(QWheelEvent *event)
{
    m_labelLinkRef->hide();
    QWebView::wheelEvent(event);
}

QWebView *WebView::createWindow(QWebPage::WebWindowType type)
{
    switch (type)
    {
        case QWebPage::WebBrowserWindow:    // Open a new tab
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
        case QWebPage::WebModalDialog:     // Open a web dialog
        {
            WebDialog *dialog = new WebDialog;
            dialog->show();
            return dialog->getView();
        }
    }
    return QWebView::createWindow(type);
}

void WebView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-browser-tab"))
    {
        event->acceptProposedAction();
        return;
    }

    QWebView::dragEnterEvent(event);
}

void WebView::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-browser-tab"))
    {
        qobject_cast<MainWindow*>(window())->dropEvent(event);
        return;
    }

    QWebView::dropEvent(event);
}

void WebView::addInspectorIfEnabled(QMenu *menu)
{
    if (!menu || !page()->settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled))
        return;

    menu->addSeparator();
    menu->addAction(tr("Inspect element"), this, &WebView::inspectElement);
}
