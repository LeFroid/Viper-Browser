#include "TabBarMimeDelegate.h"
#include "MainWindow.h"
#include "BrowserTabBar.h"
#include "BrowserTabWidget.h"
#include "WebState.h"
#include "WebWidget.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRect>
#include <QToolButton>
#include <QUrl>

TabBarMimeDelegate::TabBarMimeDelegate(MainWindow *window, BrowserTabWidget *tabWidget, BrowserTabBar *tabBar) :
    m_window(window),
    m_tabWidget(tabWidget),
    m_tabBar(tabBar),
    m_dragPixmap(),
    m_dragStartPoint(),
    m_draggedWebWidget(),
    m_externalDropInfo()
{
}

bool TabBarMimeDelegate::onDragEnter(QDragEnterEvent *dragEvent)
{
    const QMimeData *mimeData = dragEvent->mimeData();

    if (mimeData->hasUrls() || mimeData->hasFormat("application/x-browser-tab"))
    {
        dragEvent->acceptProposedAction();
        return true;
    }

    return false;
}

void TabBarMimeDelegate::onDragLeave(QDragLeaveEvent */*dragEvent*/)
{
    m_externalDropInfo.NearestTabIndex = -1;
}

void TabBarMimeDelegate::onDragMove(QDragMoveEvent *dragEvent)
{
    const QMimeData *mimeData = dragEvent->mimeData();

    if (mimeData->hasUrls())
    {
        const QPoint eventPos = dragEvent->position().toPoint();

        int nearestTabIndex = m_tabBar->tabAt(eventPos);
        if (nearestTabIndex < 0)
            nearestTabIndex = m_tabBar->count() - 1;

        const QRect r = m_tabBar->tabRect(nearestTabIndex);
        const QPoint tabCenterPos = r.center();
        const int quarterTabWidth = 0.25f * r.width();

        DropIndicatorLocation location = DropIndicatorLocation::Center;
        if (eventPos.x() > tabCenterPos.x() + quarterTabWidth)
            location = DropIndicatorLocation::Right;
        else if (eventPos.x() < tabCenterPos.x() - quarterTabWidth)
            location = DropIndicatorLocation::Left;

        m_externalDropInfo.NearestTabIndex = nearestTabIndex;
        m_externalDropInfo.Location = location;

        m_tabBar->update();
    }
}

bool TabBarMimeDelegate::onDrop(QDropEvent *dropEvent)
{
    const QMimeData *mimeData = dropEvent->mimeData();

    if (mimeData->hasUrls())
    {
        int newTabIndex = m_externalDropInfo.NearestTabIndex;

        QList<QUrl> urls = mimeData->urls();
        for (const auto &url : urls)
        {
            if (m_externalDropInfo.Location == DropIndicatorLocation::Center)
            {
                WebWidget *ww = m_tabWidget->getWebWidget(newTabIndex);
                ww->load(url);

                m_externalDropInfo.Location = DropIndicatorLocation::Right;
            }
            else
            {
                int tabOffset = (m_externalDropInfo.Location == DropIndicatorLocation::Right) ? 1 : 0;
                WebWidget *ww = m_tabWidget->newBackgroundTabAtIndex(newTabIndex + tabOffset);
                ww->load(url);

                newTabIndex += tabOffset;
            }
        }

        dropEvent->acceptProposedAction();
        m_externalDropInfo.NearestTabIndex = -1;
        return true;
    }
    else if (mimeData->hasFormat("application/x-browser-tab"))
    {
        if (static_cast<qulonglong>(m_window->winId()) != mimeData->property("tab-origin-window-id").toULongLong())
        {
            m_window->dropEvent(dropEvent);
            m_draggedWebWidget.clear();
            return true;
        }

        int originalTabIndex = m_tabWidget->indexOf(m_draggedWebWidget);
        int tabIndexAtPos = m_tabBar->tabAt(dropEvent->position().toPoint());
        if (tabIndexAtPos < 0)
        {
            auto lastTabRect = m_tabBar->tabRect(m_tabBar->count() - 1);
            if (dropEvent->position().x() >= lastTabRect.x() + lastTabRect.width())
                tabIndexAtPos = m_tabBar->count();
            else
                tabIndexAtPos = 0;
        }

        if (originalTabIndex >= 0 && tabIndexAtPos >= 0 && originalTabIndex != tabIndexAtPos)
        {
            const bool isPinned = m_tabWidget->isTabPinned(originalTabIndex);
            m_tabWidget->removeTab(originalTabIndex);
            if (tabIndexAtPos > originalTabIndex)
                --tabIndexAtPos;
            tabIndexAtPos = m_tabWidget->insertTab(tabIndexAtPos, m_draggedWebWidget, m_draggedWebWidget->getIcon(), m_draggedWebWidget->getTitle());
            m_tabBar->setTabPinned(tabIndexAtPos, isPinned);
            m_tabWidget->setCurrentIndex(tabIndexAtPos);
            m_tabBar->setCurrentIndex(tabIndexAtPos);
            m_tabBar->forceRepaint();
        }

        m_draggedWebWidget.clear();

        dropEvent->acceptProposedAction();
        return true;
    }

    return false;
}

void TabBarMimeDelegate::onMousePress(QMouseEvent *mouseEvent)
{
    int tabIdx = m_tabBar->tabAt(mouseEvent->pos());
    if (mouseEvent->button() == Qt::LeftButton
            && tabIdx >= 0)
    {
        if (WebWidget *webWidget = m_tabWidget->getWebWidget(tabIdx))
        {
            m_draggedWebWidget = webWidget;
            m_dragStartPoint = mouseEvent->pos();
            //m_draggedTabState = webWidget->getState();
            //m_draggedTabState.isPinned = isTabPinned(tabIdx);
            //m_isDraggedTabHibernating = webWidget->isHibernating();

            QRect dragTabRect = m_tabBar->tabRect(tabIdx);
            m_dragPixmap = QPixmap(dragTabRect.size());
            m_tabBar->render(&m_dragPixmap, QPoint(), QRegion(dragTabRect));
        }
    }
}

void TabBarMimeDelegate::onMouseMove(QMouseEvent *mouseEvent)
{
    if (m_draggedWebWidget.isNull())
        return;

    if (std::abs(mouseEvent->pos().y() - m_dragStartPoint.y()) < m_tabBar->height())
        return;

    // Set mime data and initiate drag event
    QDrag *drag = new QDrag(m_tabBar);
    QMimeData *mimeData = new QMimeData;

    m_dragStartPoint.setX(mouseEvent->pos().x());
    int tabIdx = m_tabBar->tabAt(m_dragStartPoint);
    const WebState &state = m_draggedWebWidget->getState();
    mimeData->setData("application/x-browser-tab", state.serialize());
    mimeData->setProperty("tab-hibernating", m_draggedWebWidget->isHibernating());
    mimeData->setProperty("tab-origin-window-id", m_window->winId());
    mimeData->setProperty("tab-index", tabIdx);
    drag->setMimeData(mimeData);
    drag->setPixmap(m_dragPixmap);

    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
    if (dropAction == Qt::MoveAction)
    {
        // If the tab was moved, and it was the only tab, close the window
        if (m_tabBar->count() == 1)
        {
            m_window->close();
        }
    }
}

void TabBarMimeDelegate::onPaint()
{
    if (m_externalDropInfo.NearestTabIndex < 0)
        return;

    QPainter p(m_tabBar);

    const QRect r = m_tabBar->tabRect(m_externalDropInfo.NearestTabIndex);
    QPoint indicatorPos(0, r.center().y());

    switch (m_externalDropInfo.Location)
    {
        case DropIndicatorLocation::Center:
            indicatorPos.setX(r.center().x());
            break;
        case DropIndicatorLocation::Left:
        {
            indicatorPos.setX(r.left());
            if (r.left() - 8 <= 0)
                indicatorPos.setX(r.left() + 8);
            break;
        }
        case DropIndicatorLocation::Right:
        {
            indicatorPos.setX(r.right());
            if (r.right() + 8 >= m_tabBar->getNewTabButton()->pos().x())
                indicatorPos.setX(r.right() - 5);
            break;
        }
    }

    p.setPen(QColor(Qt::black));
    p.setBrush(QBrush(QColor(Qt::white)));

    int cx = indicatorPos.x(), cy = indicatorPos.y() - 4;
    p.drawRect(cx - 4, cy - 8, 8, 4);

    QPainterPath path;
    path.moveTo(cx, indicatorPos.y());
    path.lineTo(std::max(0, cx - 8), cy - 4);
    path.lineTo(cx + 8, cy - 4);
    path.lineTo(cx, cy + 4);
    p.drawPath(path);

    p.fillRect(cx - 3, cy - 5, 7, 2, QBrush(QColor(Qt::white)));
}
