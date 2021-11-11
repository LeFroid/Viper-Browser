#include "AdBlockButton.h"
#include "AdBlockManager.h"
#include "MainWindow.h"
#include "Settings.h"
#include "WebView.h"
#include "WebWidget.h"

#include <algorithm>
#include <QContextMenuEvent>
#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QUrl>
#include <QWidget>
#include <QtGlobal>

AdBlockButton::AdBlockButton(bool isDarkTheme, QWidget *parent) :
    QToolButton(parent),
    m_adBlockManager(nullptr),
    m_settings(nullptr),
    m_mainWindow(nullptr),
    m_icon(isDarkTheme ? QStringLiteral(":/AdBlock-white.svg") : QStringLiteral(":/AdBlock.svg")),
    m_timer(),
    m_lastCount(0)
{
    setIcon(m_icon);
    setStyleSheet(QLatin1String("QToolButton { margin-right: 6px; }"));
    setToolTip(tr("No ads blocked on this page"));

    connect(&m_timer, &QTimer::timeout, this, &AdBlockButton::updateCount);
    m_timer.start(500);
}

void AdBlockButton::setAdBlockManager(adblock::AdBlockManager *adBlockManager)
{
    m_adBlockManager = adBlockManager;
}

void AdBlockButton::setSettings(Settings *settings)
{
    m_settings = settings;
}

void AdBlockButton::updateCount()
{
    if (!isVisible())
        return;

    if (!m_mainWindow)
        m_mainWindow = qobject_cast<MainWindow*>(window());

    if (!m_mainWindow || m_mainWindow->isMinimized() || !m_mainWindow->isActiveWindow())
        return;


    if (WebWidget *ww = m_mainWindow->currentWebWidget())
    {
        const int adBlockCount = m_adBlockManager ? m_adBlockManager->getNumberAdsBlocked(ww->url().adjusted(QUrl::RemoveFragment)) : 0;
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
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        font.setPointSize(adBlockCount >= 100 ? 9 : 10);
#else
        font.setPointSize(adBlockCount >= 100 ? 11 : 14);
#endif
        //font.setBold(true);
        painter.setFont(font);

        QFontMetrics metrics(font);

        // Draw rect containing the count
        const int startX = adBlockPixmap.width() / 3,
                  startY = adBlockPixmap.height() / 2;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        const int containerWidth = adBlockPixmap.width() - startX;
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        const int containerWidth = std::min(metrics.horizontalAdvance(numAdsBlocked) * 3, adBlockPixmap.width()) - startX;
#else
        const int containerWidth = std::min(metrics.width(numAdsBlocked) * 3, adBlockPixmap.width()) - startX;
#endif

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        const int containerHeight = adBlockPixmap.height() - startY;

        const int textWidth = metrics.horizontalAdvance(numAdsBlocked);
        const int startTextX = startX + ((containerWidth - textWidth) / 2);
        QPoint textPos(startTextX, height() - 3);
#else
        const int containerHeight = std::min(metrics.height() + 6, adBlockPixmap.height() - startY);
        QPoint textPos(startX + 1, startY + containerHeight - 2);
        if (adBlockCount < 10)
            textPos.setX(startX + containerWidth / 4);
#endif
        const QRect infoRect(startX, startY, containerWidth, containerHeight);
        painter.fillRect(infoRect, QBrush(QColor(67, 67, 67)));

        // Draw the number of ads being blocked
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(textPos, numAdsBlocked);

        // Show the updated icon
        QIcon renderedIcon(adBlockPixmap);
        setIcon(renderedIcon);
        setToolTip(QString("%1 ads blocked on this page").arg(numAdsBlocked));
    }
}

void AdBlockButton::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);
    menu->addAction(tr("Manage Subscriptions"), this, [this](){
        emit manageSubscriptionsRequest();
    });
    menu->addAction(tr("View Logs"), this, [this](){
        emit viewLogsRequest();
    });
    if (m_settings)
    {
        menu->addSeparator();
        const bool isAdBlockEnabled = m_settings->getValue(BrowserSetting::AdBlockPlusEnabled).toBool();
        const QString toggleText = isAdBlockEnabled ? tr("Disable Advertisement Blocker") : tr("Enable Advertisement Blocker");
        menu->addAction(toggleText, this, [this, isAdBlockEnabled](){
            m_settings->setValue(BrowserSetting::AdBlockPlusEnabled, !isAdBlockEnabled);
        });
    }
    menu->exec(event->globalPos());
}
