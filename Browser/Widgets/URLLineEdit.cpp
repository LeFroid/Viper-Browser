#include "BrowserApplication.h"
#include "SecurityManager.h"
#include "URLLineEdit.h"
#include "URLSuggestionModel.h"

#include <QCompleter>
#include <QIcon>
#include <QResizeEvent>
#include <QSize>
#include <QStyle>
#include <QToolButton>

URLLineEdit::URLLineEdit(QWidget *parent) :
    QLineEdit(parent)
{
    QSizePolicy policy = sizePolicy();
    setSizePolicy(QSizePolicy::MinimumExpanding, policy.verticalPolicy());

    // Set completion model
    QCompleter *urlCompleter = new QCompleter(parent);
    urlCompleter->setModel(sBrowserApplication->getURLSuggestionModel());
    urlCompleter->setMaxVisibleItems(10);
    urlCompleter->setCompletionColumn(0);
    urlCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    urlCompleter->setFilterMode(Qt::MatchContains);
    urlCompleter->setCompletionMode(QCompleter::PopupCompletion);
    setCompleter(urlCompleter);

    // URL results are in format "url - title", so a slot must be added to extract the URL
    // from the rest of the model's results
    connect(this, &URLLineEdit::textChanged, [=](const QString &text){
        int completedIdx = text.indexOf(" - ");
        if (completedIdx >= 0)
        {
            setText(text.left(completedIdx));
        }
    });

    // Style common to both tool buttons in line edit
    const QString toolButtonStyle = QStringLiteral("QToolButton { border: none; padding: 0px; }");

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
}

URLLineEdit::~URLLineEdit()
{
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
            m_bookmarkButton->setToolTip(tr("Remove this bookmark"));
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

    SecurityIcon secureIcon = SecurityIcon::Standard;
    if (url.scheme().compare(QStringLiteral("https")) == 0)
        secureIcon = SecurityManager::instance().isInsecure(url.host()) ? SecurityIcon::Insecure : SecurityIcon::Secure;
    setSecurityIcon(secureIcon);
}

void URLLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);

    const QSize securitySize = m_securityButton->sizeHint();
    const QSize bookmarkSize = m_bookmarkButton->sizeHint();
    const QRect widgetRect = rect();

    m_securityButton->move(widgetRect.left(), (widgetRect.bottom() + 1 - securitySize.height()) / 2);
    m_bookmarkButton->move(widgetRect.right() - bookmarkSize.width() - 2, (widgetRect.bottom() + 1 - bookmarkSize.height()) / 2);
}
