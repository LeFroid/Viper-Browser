#include "BrowserApplication.h"
#include "URLSuggestionItemDelegate.h"
#include "URLSuggestionListModel.h"

#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QtGlobal>

#include <QDebug>

URLSuggestionItemDelegate::URLSuggestionItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent),
    m_padding(6)
{
}

void URLSuggestionItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption = option;
    initStyleOption(&itemOption, index);
    itemOption.showDecorationSelected = true;

    const QRect itemRect = itemOption.rect;
    const int cy = itemRect.top() + itemRect.height() / 2;

    const bool isSelectedItem = (itemOption.state & QStyle::State_Selected);
    if (isSelectedItem)
        painter->fillRect(itemRect, QColor(61, 113, 255));

    const bool isMouseOver = (itemOption.state & QStyle::State_MouseOver);
    if (isMouseOver)
        painter->fillRect(itemRect, QColor(111, 173, 255));

    // Draw favicon
    QRect faviconRect(itemRect.left() + m_padding, cy - 8, 16, 16);
    QIcon favicon = index.data(URLSuggestionListModel::Favicon).value<QIcon>();
    painter->drawPixmap(faviconRect, favicon.pixmap(16, 16));

    // Draw title
    QFont titleFont = itemOption.font;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    titleFont.setPointSize(12);
#else
    titleFont.setPointSize(14);
#endif
    painter->setFont(titleFont);

    QPen titlePen = painter->pen();
    QBrush titleBrush = (isSelectedItem || isMouseOver) ? QApplication::palette().highlightedText() : QApplication::palette().text();
    titlePen.setBrush(titleBrush);
    painter->setPen(titlePen);

    QFontMetrics titleFontMetrics(titleFont);
    const QPointF titlePos(faviconRect.right() + 2 * m_padding, cy - 1 - titleFontMetrics.height() / 2);
    const QRectF titleRect(titlePos, QSize(itemRect.width() * 0.4f, titleFontMetrics.height()));
    QString title = index.data(URLSuggestionListModel::Title).toString().append(QLatin1String(" -"));
    title = titleFontMetrics.elidedText(title, Qt::ElideRight, titleRect.width());
    painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);

    // Draw url next to title
    QFont urlFont = itemOption.font;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
    urlFont.setPointSize(10);
#else
    urlFont.setPointSize(11);
#endif
    painter->setFont(urlFont);

    QPen urlPen = painter->pen();
    QBrush urlBrush;
    if (isSelectedItem || isMouseOver)
        urlBrush = QBrush(QColor(255, 255, 255));
    else
        urlBrush = sBrowserApplication->isDarkTheme() ? QBrush(QColor(0, 127, 255)) : QBrush(QColor(0, 0, 238));
    urlPen.setBrush(urlBrush);
    painter->setPen(urlPen);

    QFontMetrics urlFontMetrics(urlFont);
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
    const QPointF urlPos(titlePos.x() + titleFontMetrics.horizontalAdvance(title) + m_padding,
                         cy - 1 - urlFontMetrics.height() / 2);
#else
    const QPointF urlPos(titlePos.x() + titleFontMetrics.width(title) + m_padding,
                         cy - 1 - urlFontMetrics.height() / 2);
#endif
    const QRectF urlRect(urlPos, QSize(itemRect.width() - m_padding - urlPos.x() - faviconRect.width(), urlFontMetrics.height()));
    QString url = index.data(URLSuggestionListModel::Link).toString();
    url = urlFontMetrics.elidedText(url, Qt::ElideRight, urlRect.width());
    painter->drawText(urlRect, Qt::AlignLeft | Qt::AlignVCenter, url);

    // Draw bookmark icon if bookmarked
    if (index.data(URLSuggestionListModel::Bookmark).toBool())
    {
        QRect bookmarkRect(urlRect.right() + m_padding, cy - 8, 16, 16);
        QIcon bookmarkIcon = (isSelectedItem || isMouseOver) ? QIcon(QLatin1String(":/not_bookmarked.png")) : QIcon(QLatin1String(":/bookmarked.png"));
        painter->drawPixmap(bookmarkRect, bookmarkIcon.pixmap(16, 16));
    }

    // Draw a line under item
    if (!isSelectedItem)
    {
        painter->setPen(QPen(QColor(164, 164, 164)));
        painter->drawLine(itemRect.left(), itemRect.bottom(), itemRect.right(), itemRect.bottom());
    }
}

QSize URLSuggestionItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize hint = QStyledItemDelegate::sizeHint(option, index);
    if (hint.height() < 24)
        hint.setHeight(24);
    return hint;
}
