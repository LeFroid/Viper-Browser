#include <algorithm>
#include "WebLinkLabel.h"
#include "WebPage.h"

WebLinkLabel::WebLinkLabel(QWidget *parent, WebPage *page) :
    QLabel(parent),
    m_showingLink(false)
{
    hide();
    setStyleSheet("QLabel { background-color: #FFFFFF; border: 1px solid #CCCCCC; }");
    setMinimumSize(QSize(0, 0));
    setAutoFillBackground(false);
    setFrameShape(QLabel::Box);
    setTextInteractionFlags(Qt::NoTextInteraction);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    if (QWidget *mainWidget = parent->parentWidget())
        move(QPoint(0, std::max(mainWidget->geometry().height() - 17, 0)));

    connect(page, &WebPage::linkHovered, this, &WebLinkLabel::showLinkRef);
}

QSize WebLinkLabel::sizeHint() const
{
    QSize hint = QLabel::sizeHint();

    QString urlText = text();
    if (!urlText.isEmpty())
    {
        QFontMetrics fMetric = fontMetrics();
        return hint.expandedTo(
                    QSize(fMetric.width(urlText) + fMetric.width("R") * 2, hint.height()));
    }

    return hint;
}

void WebLinkLabel::showLinkRef(const QString &link, const QString &/*title*/, const QString &/*context*/)
{
    // Hide tooltip if parameter is empty
    if (link.isEmpty())
    {
        setText(QString());
        m_showingLink = false;
        hide();
        return;
    }

    setText(link);
    if (m_showingLink)
        hide();
    show();
    m_showingLink = true;
}
