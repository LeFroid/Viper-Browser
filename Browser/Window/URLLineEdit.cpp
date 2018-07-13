#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SecurityManager.h"
#include "URLLineEdit.h"
#include "URLSuggestionWidget.h"
#include "WebWidget.h"

#include <QCompleter>
#include <QIcon>
#include <QResizeEvent>
#include <QSize>
#include <QStyle>
#include <QToolButton>

//todo: add icon to show when cookies are being blocked on the current website
URLLineEdit::URLLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_securityButton(nullptr),
    m_bookmarkButton(nullptr),
    m_userTextMap(),
    m_activeWebView(nullptr),
    m_suggestionWidget(nullptr)
{
    setObjectName(QLatin1String("urlLineEdit"));
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    QFont lineEditFont = font();
    lineEditFont.setPointSize(lineEditFont.pointSize() + 1);
    setFont(lineEditFont);

    // Style common to both tool buttons in line edit
    const QString toolButtonStyle = QLatin1String("QToolButton { border: none; padding: 0px; } QToolButton:hover { background-color: #E6E6E6; }");

    // Setup tool button
    m_securityButton = new QToolButton(this);
    m_securityButton->setCursor(Qt::ArrowCursor);
    m_securityButton->setStyleSheet(toolButtonStyle);
    connect(m_securityButton, &QToolButton::clicked, this, &URLLineEdit::viewSecurityInfo);

    // Setup bookmark button
    m_bookmarkButton = new QToolButton(this);
    m_bookmarkButton->setCursor(Qt::ArrowCursor);
    m_bookmarkButton->setStyleSheet(toolButtonStyle);
    connect(m_bookmarkButton, &QToolButton::clicked, this, &URLLineEdit::toggleBookmarkStatus);

    // Set appearance
    QString urlStyle = QString("QLineEdit { border: 1px solid #666666; padding-left: %1px; padding-right: %2px; }"
                               "QLineEdit:focus { border: 1px solid #6c91ff; }");
    urlStyle = urlStyle.arg(m_securityButton->sizeHint().width()).arg(m_bookmarkButton->sizeHint().width());
    setStyleSheet(urlStyle);
    setPlaceholderText(tr("Search or enter address"));

    // Setup suggestion widget
    m_suggestionWidget = new URLSuggestionWidget;
    m_suggestionWidget->setURLLineEdit(this);
    connect(m_suggestionWidget, &URLSuggestionWidget::urlChosen, this, &URLLineEdit::onSuggestedURLChosen);
    connect(this, &URLLineEdit::editingFinished, m_suggestionWidget, &URLSuggestionWidget::hide);

    // Setup text editing handler
    connect(this, &URLLineEdit::textEdited, this, &URLLineEdit::onTextEdited);
}

URLLineEdit::~URLLineEdit()
{
    delete m_suggestionWidget;
}

void URLLineEdit::setCurrentPageBookmarked(bool isBookmarked)
{
    setBookmarkIcon(isBookmarked ? BookmarkIcon::Bookmarked : BookmarkIcon::NotBookmarked);
}

void URLLineEdit::setBookmarkIcon(BookmarkIcon iconType)
{
    switch (iconType)
    {
        case BookmarkIcon::NoIcon:
            m_bookmarkButton->setIcon(QIcon());
            m_bookmarkButton->setToolTip(QString());
            return;
        case BookmarkIcon::Bookmarked:
            m_bookmarkButton->setIcon(QIcon(":/bookmarked.png"));
            m_bookmarkButton->setToolTip(tr("Edit this bookmark"));
            return;
        case BookmarkIcon::NotBookmarked:
            m_bookmarkButton->setIcon(QIcon(":/not_bookmarked.png"));
            m_bookmarkButton->setToolTip(tr("Bookmark this page"));
            return;
    }
}

void URLLineEdit::setSecurityIcon(SecurityIcon iconType)
{
    switch (iconType)
    {
        case SecurityIcon::Standard:
            m_securityButton->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
            m_securityButton->setToolTip(tr("Standard HTTP connection - insecure"));
            return;
        case SecurityIcon::Secure:
            m_securityButton->setIcon(QIcon(":/https_secure.png"));
            m_securityButton->setToolTip(tr("Secure connection"));
            return;
        case SecurityIcon::Insecure:
            m_securityButton->setIcon(QIcon(":/https_insecure.png"));
            m_securityButton->setToolTip(tr("Insecure connection"));
            return;
    }
}

void URLLineEdit::setURL(const QUrl &url)
{
    setText(url.toString(QUrl::FullyEncoded));

    const QString scheme = url.scheme();
    SecurityIcon secureIcon = SecurityIcon::Standard;

    if (scheme == QLatin1String("https") || scheme == QLatin1String("viper"))
        secureIcon = SecurityManager::instance().isInsecure(url.host()) ? SecurityIcon::Insecure : SecurityIcon::Secure;

    setSecurityIcon(secureIcon);

    if (url.isValid())
        setURLFormatted(url);
}

void URLLineEdit::setURLFormatted(const QUrl &url)
{
    // Color in the line edit based on url and security information
    std::vector<QTextLayout::FormatRange> urlTextFormat;
    QTextCharFormat schemeFormat, hostFormat, pathFormat;
    QTextLayout::FormatRange schemeFormatRange, hostFormatRange, pathFormatRange;
    schemeFormatRange.start = 0;

    schemeFormat.setForeground(QBrush(QColor(110, 110, 110)));
    hostFormat.setForeground(QBrush(Qt::black));
    pathFormat.setForeground(QBrush(QColor(110, 110, 110)));

    const bool isHttps = url.scheme().compare(QLatin1String("https")) == 0;
    if (isHttps)
    {
        if (SecurityManager::instance().isInsecure(url.host()))
            schemeFormat.setForeground(QBrush(QColor(157, 28, 28)));
        else
            schemeFormat.setForeground(QBrush(QColor(11, 128, 67)));
    }

    schemeFormatRange.length = url.scheme().size();
    schemeFormatRange.format = schemeFormat;
    urlTextFormat.push_back(schemeFormatRange);

    QTextLayout::FormatRange greyFormatRange;
    greyFormatRange.start = schemeFormatRange.length;
    greyFormatRange.length = 3;
    greyFormatRange.format = pathFormat;
    urlTextFormat.push_back(greyFormatRange);

    const QString urlText = text();

    hostFormatRange.start = greyFormatRange.start + 3;
    hostFormatRange.length = url.host(QUrl::FullyEncoded).size();
    if (hostFormatRange.length == 0)
        hostFormatRange.length = std::max(urlText.indexOf(QLatin1Char('/'), hostFormatRange.start), urlText.size() - hostFormatRange.start);
    hostFormatRange.format = hostFormat;
    urlTextFormat.push_back(hostFormatRange);

    pathFormatRange.start = hostFormatRange.start + hostFormatRange.length;
    pathFormatRange.length = urlText.size() - pathFormatRange.start;
    pathFormatRange.format = pathFormat;
    urlTextFormat.push_back(pathFormatRange);

    setTextFormat(urlTextFormat);
}

// https://stackoverflow.com/questions/14417333/how-can-i-change-color-of-part-of-the-text-in-qlineedit/14424003#14424003
void URLLineEdit::setTextFormat(const std::vector<QTextLayout::FormatRange> &formats)
{
    QList<QInputMethodEvent::Attribute> attributes;
    for (const QTextLayout::FormatRange& fmt : formats)
    {
        QInputMethodEvent::AttributeType type = QInputMethodEvent::TextFormat;
        int start = fmt.start - cursorPosition();
        int length = fmt.length;
        QVariant value = fmt.format;
        attributes.append(QInputMethodEvent::Attribute(type, start, length, value));
    }
    QInputMethodEvent event(QString(), attributes);
    QCoreApplication::sendEvent(this, &event);
}

void URLLineEdit::removeMappedView(WebWidget *view)
{
    auto it = m_userTextMap.find(view);
    if (it != m_userTextMap.end())
        m_userTextMap.erase(it);
}

void URLLineEdit::onSuggestedURLChosen(const QUrl &url)
{
    setURL(url);
    emit returnPressed();
}

void URLLineEdit::onTextEdited(const QString &text)
{
    if (MainWindow *mw = qobject_cast<MainWindow*>(window()))
    {
        const QUrl currentViewUrl = mw->currentWebWidget()->url();
        if (currentViewUrl.toString(QUrl::FullyEncoded).compare(text) == 0)
        {
            setURLFormatted(currentViewUrl);
            return;
        }

        m_suggestionWidget->suggestForInput(text);
        if (!m_suggestionWidget->isVisible())
            m_suggestionWidget->alignAndShow(mapToGlobal(pos()), frameGeometry());
    }
    setTextFormat(std::vector<QTextLayout::FormatRange>());
}

void URLLineEdit::tabChanged(WebWidget *newView)
{
    if (m_activeWebView != nullptr)
        m_userTextMap[m_activeWebView] = text();

    auto it = m_userTextMap.find(newView);
    if (it != m_userTextMap.end())
    {
        setText(it->second);
        setTextFormat(std::vector<QTextLayout::FormatRange>());
    }
    else
        setURL(QUrl());

    m_activeWebView = newView;
}

void URLLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);

    const QSize securitySize = m_securityButton->sizeHint();
    const QSize bookmarkSize = m_bookmarkButton->sizeHint();
    const QRect widgetRect = rect();

    m_securityButton->move(widgetRect.left() + 1, (widgetRect.bottom() + 1 - securitySize.height()) / 2);
    m_bookmarkButton->move(widgetRect.right() - bookmarkSize.width() - 1, (widgetRect.bottom() + 1 - bookmarkSize.height()) / 2);

    m_suggestionWidget->hide();
    m_suggestionWidget->needResizeWidth(event->size().width());
}
