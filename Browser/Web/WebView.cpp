#include <algorithm>
#include <QContextMenuEvent>
#include <QFile>
#include <QLabel>
#include <QMenu>
#include <QWebHitTestResult>
#include <QWheelEvent>

#include "BrowserApplication.h"
#include "DownloadManager.h"
#include "Settings.h"
#include "WebDialog.h"
#include "WebView.h"
#include "WebPage.h"

WebView::WebView(QWidget *parent) :
    QWebView(parent),
    m_page(new WebPage(this)),
    m_progress(0)
{
    setPage(m_page);

    // Setup link hover label
    m_labelLinkRef = new QLabel(this);
    m_labelLinkRef->hide();
    m_labelLinkRef->setStyleSheet("QLabel { background-color: #FFFFFF; border: 1px solid #CCCCCC; }");
    m_labelLinkRef->setMinimumSize(QSize(0, 0));
    m_labelLinkRef->setAutoFillBackground(false);
    m_labelLinkRef->setFrameShape(QLabel::Box);
    m_labelLinkRef->setTextInteractionFlags(Qt::NoTextInteraction);
    m_labelLinkRef->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    QPoint labelPos(0, std::max(parent->geometry().height() - 17, 0));
    m_labelLinkRef->move(labelPos);

    connect(this, &WebView::loadProgress, [=](int value){
       m_progress = value;
    });

    // Connect QWebPage signals to slots in the web view
    connect(m_page, &WebPage::linkHovered, this, &WebView::showLinkRef);
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

void WebView::requestDownload(const QNetworkRequest &request)
{
    BrowserApplication *app = sBrowserApplication;
    bool askWhereToSave = app->getSettings()->getValue("AskWhereToSaveDownloads").toBool();
    app->getDownloadManager()->download(request, askWhereToSave);
}

void WebView::showLinkRef(const QString &link, const QString &/*title*/, const QString &/*context*/)
{
    // Hide tooltip if parameter is empty
    if (link.isEmpty())
    {
        m_labelLinkRef->hide();
        return;
    }

    m_labelLinkRef->setText(link);
    m_labelLinkRef->setMaximumWidth(m_labelLinkRef->fontMetrics().boundingRect(link).width() + 15);
    m_labelLinkRef->show();
}

void WebView::contextMenuEvent(QContextMenuEvent *event)
{
    QWebHitTestResult res = page()->mainFrame()->hitTestContent(event->pos());

    // Check if context menu includes links
    if (!res.linkUrl().isEmpty())
    {
        QMenu menu(this);
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
        QMenu menu(this);
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
        QMenu menu(this);
        menu.addAction(pageAction(QWebPage::Copy));
        menu.addAction(tr("Search for selected text"));
        addInspectorIfEnabled(&menu);
        menu.exec(mapToGlobal(event->pos()));
        return;
    }
    // Default menu
    QMenu menu(this);
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

QWebView *WebView::createWindow(QWebPage::WebWindowType /*type*/)
{
    WebDialog *dialog = new WebDialog;
    dialog->show();
    return dialog->getView();
    //todo: create another mainwindow via browserapplication, get the tab widget's active widget, cast to a webview and return
    //return QWebView::createWindow(type);
}

void WebView::addInspectorIfEnabled(QMenu *menu)
{
    if (!menu || !page()->settings()->testAttribute(QWebSettings::DeveloperExtrasEnabled))
        return;

    menu->addSeparator();
    menu->addAction(tr("Inspect element"), this, &WebView::inspectElement);
}
