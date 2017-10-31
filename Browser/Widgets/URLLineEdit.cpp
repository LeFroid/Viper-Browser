#include "BrowserApplication.h"
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

    // Setup tool button
    m_securityButton = new QToolButton(this);
    m_securityButton->setCursor(Qt::ArrowCursor);
    m_securityButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    connect(m_securityButton, &QToolButton::clicked, this, &URLLineEdit::viewSecurityInfo);

    // Set appearance
    setStyleSheet(QString("QLineEdit { border: 1px solid #666666; padding-left: %1px; }"
                          "QLineEdit:focus { border: 1px solid #6c91ff; }").arg(m_securityButton->sizeHint().width()));
    setPlaceholderText(tr("Search or enter address"));
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

void URLLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);

    QSize sz = m_securityButton->sizeHint();
    const QRect widgetRect = rect();
    m_securityButton->move(widgetRect.left(), (widgetRect.bottom() + 1 - sz.height()) / 2);
}
