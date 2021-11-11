#include "BookmarkNode.h"
#include "BookmarkManager.h"
#include "BrowserApplication.h"
#include "MainWindow.h"
#include "SchemeRegistry.h"
#include "SecurityManager.h"
#include "URLLineEdit.h"
#include "URLSuggestionWidget.h"
#include "WebWidget.h"

#include <array>
#include <QApplication>
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
    m_suggestionWidget(nullptr),
    m_bookmarkManager(nullptr),
    m_bookmarkNode(nullptr),
    m_isDarkTheme(sBrowserApplication->isDarkTheme())
{
    setObjectName(QLatin1String("urlLineEdit"));

    QFont lineEditFont = font();
    lineEditFont.setPointSize(lineEditFont.pointSize() + 1);
    setFont(lineEditFont);

    // Style common to both tool buttons in line edit
    const QString toolButtonStyle = QLatin1String("QToolButton { border: none; padding: 0px; } "
                                                  "QToolButton:hover { background-color: #E6E6E6; }");

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
    QString urlStyle = QString("QLineEdit { border: 1px solid #919191; border-radius: 2px; padding-left: %1px; padding-right: %2px; } "
                               "QLineEdit:focus { border: 1px solid #6c91ff; }");
    setStyleSheet(urlStyle.arg(m_securityButton->sizeHint().width()).arg(m_bookmarkButton->sizeHint().width()));
    setPlaceholderText(tr("Search or enter address"));

    // Setup suggestion widget
    m_suggestionWidget = new URLSuggestionWidget;
    m_suggestionWidget->setURLLineEdit(this);
    connect(m_suggestionWidget, &URLSuggestionWidget::urlChosen, this, &URLLineEdit::onSuggestedURLChosen);
    connect(m_suggestionWidget, &URLSuggestionWidget::noSuggestionChosen, this, &URLLineEdit::tryToFormatInput);
    connect(this, &URLLineEdit::editingFinished, m_suggestionWidget, &URLSuggestionWidget::hide);

    // Setup text editing handler
    connect(this, &URLLineEdit::textEdited, this, &URLLineEdit::onTextEdited);

    // Setup Enter action handler
    connect(this, &URLLineEdit::returnPressed, this, &URLLineEdit::onInputEntered);
}

URLLineEdit::~URLLineEdit()
{
    delete m_suggestionWidget;
}

BookmarkNode *URLLineEdit::getBookmarkNode() const
{
    return m_bookmarkNode;
}

void URLLineEdit::setCurrentPageBookmarked(bool isBookmarked, BookmarkNode *node)
{
    m_bookmarkNode = node;
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
            m_bookmarkButton->setIcon(QIcon(QStringLiteral(":/bookmarked.png")));
            m_bookmarkButton->setToolTip(tr("Edit this bookmark"));
            return;
        case BookmarkIcon::NotBookmarked:
            m_bookmarkButton->setIcon(QIcon(QStringLiteral(":/not_bookmarked.png")));
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
            m_securityButton->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/https_secure_bright.png") : QStringLiteral(":/https_secure.png")));
            m_securityButton->setToolTip(tr("Secure connection"));
            return;
        case SecurityIcon::Insecure:
            m_securityButton->setIcon(QIcon(m_isDarkTheme ? QStringLiteral(":/https_insecure_bright.png") : QStringLiteral(":/https_insecure.png")));
            m_securityButton->setToolTip(tr("Insecure connection"));
            return;
    }
}

void URLLineEdit::setURL(const QUrl &url)
{
    setText(url.toString(QUrl::FullyEncoded));

    const QString scheme = url.scheme();
    SecurityIcon secureIcon = SecurityIcon::Standard;

    if (SchemeRegistry::isSecure(scheme))
        secureIcon = SecurityManager::instance().isInsecure(url.host()) ? SecurityIcon::Insecure : SecurityIcon::Secure;

    setSecurityIcon(secureIcon);

    if (url.isValid())
        setURLFormatted(url);
}

void URLLineEdit::setURLFormatted(const QUrl &url)
{
    const QString urlText = text();

    // Color in the line edit based on url and security information
    std::vector<QTextLayout::FormatRange> urlTextFormat;
    QTextCharFormat schemeFormat, hostFormat, pathFormat;
    QTextLayout::FormatRange schemeFormatRange, hostFormatRange, pathFormatRange;
    schemeFormatRange.start = 0;

    schemeFormat.setForeground(QBrush(QColor(110, 110, 110)));
    hostFormat.setForeground(QApplication::palette().text());
    pathFormat.setForeground(QBrush(QColor(110, 110, 110)));

    if (SchemeRegistry::isSecure(url.scheme()))
    {
        if (SecurityManager::instance().isInsecure(url.host()))
            schemeFormat.setForeground(QBrush(m_isDarkTheme ? QColor(210, 56, 62) : QColor(157, 28, 28)));
        else
            schemeFormat.setForeground(QBrush(m_isDarkTheme ? QColor(39, 174, 96) : QColor(11, 128, 67)));
    }

    schemeFormatRange.length = url.scheme().size();
    schemeFormatRange.format = schemeFormat;

    hostFormatRange.start = 0;

    if (schemeFormatRange.length > 0 && urlText.startsWith(url.scheme()))
    {
        urlTextFormat.push_back(schemeFormatRange);

        QTextLayout::FormatRange greyFormatRange;
        greyFormatRange.start = schemeFormatRange.length;
        greyFormatRange.length = 3;
        greyFormatRange.format = pathFormat;
        urlTextFormat.push_back(greyFormatRange);

        hostFormatRange.start = greyFormatRange.start + 3;
    }

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

void URLLineEdit::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_bookmarkManager = serviceLocator.getServiceAs<BookmarkManager>("BookmarkManager");
    m_suggestionWidget->setServiceLocator(serviceLocator);
}

void URLLineEdit::removeMappedView(WebWidget *view)
{
    auto it = m_userTextMap.find(view);
    if (it != m_userTextMap.end())
        it = m_userTextMap.erase(it);
}

void URLLineEdit::onSuggestedURLChosen(const QUrl &url)
{
    setURL(url);
    emit loadRequested(url);
}

void URLLineEdit::onInputEntered()
{
    QString urlText = text();
    if (urlText.isEmpty())
        return;

    QUrl location = QUrl::fromUserInput(urlText);
    const QString urlHost = location.host();
    if (location.isValid() && (urlHost.contains(QStringLiteral("."))
                || urlHost.compare(QStringLiteral("localhost"), Qt::CaseInsensitive) == 0))
    {
        setURL(location);
        emit loadRequested(location);
        return;
    }

    QString urlTextStart = urlText;
    int delimIdx = urlTextStart.indexOf(QLatin1Char(' '));
    if (delimIdx > 0)
        urlTextStart = urlTextStart.left(delimIdx);

    if (isInputForBookmarkShortcut(urlText))
        return;

    setURL(location);
    emit loadRequested(location);
}

bool URLLineEdit::isInputForBookmarkShortcut(const QString &input)
{
    if (!m_bookmarkManager)
        return false;

    QString urlText = input,
            urlTextStart = input;

    int delimIdx = urlTextStart.indexOf(QLatin1Char(' '));
    if (delimIdx > 0)
        urlTextStart = urlTextStart.left(delimIdx);

    for (auto it : *m_bookmarkManager)
    {
        if (it->getType() == BookmarkNode::Bookmark
                && (urlTextStart.compare(it->getShortcut()) == 0 || urlText.compare(it->getShortcut()) == 0))
        {
            QString bookmarkUrl = it->getURL().toString(QUrl::FullyEncoded);
            if (delimIdx > 0 && bookmarkUrl.contains(QLatin1String("%25s")))
                urlText = bookmarkUrl.replace(QLatin1String("%25s"), urlText.mid(delimIdx + 1));
            else
                urlText = bookmarkUrl;

            QUrl location = QUrl::fromUserInput(urlText);
            setURL(location);
            emit loadRequested(location);
            return true;
        }
    }

    return false;
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
    }

    m_suggestionWidget->suggestForInput(text);

    const int cursorPos = cursorPosition();
    tryToFormatInput(text);
    setCursorPosition(cursorPos);
}

void URLLineEdit::tryToFormatInput(const QString &text)
{
    setText(text);
    QUrl maybeUrl = QUrl::fromUserInput(text);
    if (maybeUrl.isValid()
            && !maybeUrl.scheme().isEmpty())
        setURLFormatted(maybeUrl);
    else
        setTextFormat(std::vector<QTextLayout::FormatRange>());
}

void URLLineEdit::tabChanged(WebWidget *newView)
{
    if (m_activeWebView != nullptr)
        m_userTextMap[m_activeWebView] = text();

    auto it = m_userTextMap.find(newView);
    if (it != m_userTextMap.end() && newView->isOnBlankPage())
    {
        setText(it->second);
        setTextFormat(std::vector<QTextLayout::FormatRange>());
    }
    else
        setURL(newView->url());

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
