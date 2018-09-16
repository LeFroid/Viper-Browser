#include "AdBlockButton.h"
#include "AdBlockManager.h"
#include "MainWindow.h"
#include "WebView.h"
#include "WebWidget.h"

#include <algorithm>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QPixmap>
#include <QUrl>
#include <QWidget>

AdBlockButton::AdBlockButton(QWidget *parent) :
    QToolButton(parent),
    m_icon(QLatin1String(":/AdBlock.svg")),
    m_timer(),
    m_lastCount(0)
{
    setIcon(m_icon);
    setStyleSheet(QLatin1String("QToolButton { margin-right: 6px; }"));
    setToolTip(tr("No ads blocked on this page"));

    connect(&m_timer, &QTimer::timeout, this, &AdBlockButton::updateCount);
    m_timer.start(500);
}

void AdBlockButton::updateCount()
{
    if (!isVisible())
        return;

    if (MainWindow *win = qobject_cast<MainWindow*>(window()))
    {
        if (win->isMinimized())
            return;

        if (WebWidget *ww = win->currentWebWidget())
        {
            const int adBlockCount = AdBlockManager::instance().getNumberAdsBlocked(ww->url());
            if (adBlockCount == m_lastCount)
                return;

            m_lastCount = adBlockCount;

            if (adBlockCount == 0)
            {
                setIcon(m_icon);
                setToolTip(tr("No ads blocked on this page"));
                return;
            }

            QString numAdsBlocked = QString::number(adBlockCount);

            // Draw the count inside a box towards the bottom of the ad block icon
            QPixmap adBlockPixmap = m_icon.pixmap(width(), height());
            QPainter painter(&adBlockPixmap);

            // Setup font
            QFont font = painter.font();
            font.setPointSize(14);
            //font.setBold(true);
            painter.setFont(font);

            QFontMetrics metrics(font);

            // Draw rect containing the count
            const int startX = adBlockPixmap.width() / 3, startY = adBlockPixmap.height() / 2;
            const int containerWidth = std::min(metrics.width(numAdsBlocked) * 3, adBlockPixmap.width()) - startX;
            const int containerHeight = std::min(metrics.height() + 4, adBlockPixmap.height() - startY);
            const QRect infoRect(startX, startY, containerWidth, containerHeight);
            painter.fillRect(infoRect, QBrush(QColor(67, 67, 67)));

            // Draw the number of ads being blocked
            QPoint textPos(startX + 1, startY + containerHeight - 2);
            if (adBlockCount < 10)
                textPos.setX(startX + containerWidth / 4);
            painter.setPen(QColor(255, 255, 255));
            painter.drawText(textPos, numAdsBlocked);

            // Show the updated icon
            QIcon renderedIcon(adBlockPixmap);
            setIcon(renderedIcon);
            setToolTip(QString("%1 ads blocked on this page").arg(numAdsBlocked));
        }
    }
}
