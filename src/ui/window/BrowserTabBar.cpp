#include "BrowserApplication.h"
#include "BrowserTabBar.h"
#include "BrowserTabWidget.h"
#include "MainWindow.h"
#include "WebPage.h"
#include "WebView.h"
#include "WebWidget.h"

#include <QApplication>
#include <QDrag>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShortcut>
#include <QStylePainter>
#include <QToolButton>

BrowserTabBar::BrowserTabBar(QWidget *parent) :
    QTabBar(parent),
    m_buttonNewTab(nullptr),
    m_dragStartPos(),
    m_draggedTabState(),
    m_isDraggedTabHibernating(false),
    m_dragPixmap(),
    m_externalDropInfo(),
    m_mimeDelegate(qobject_cast<MainWindow*>(parent->window()), qobject_cast<BrowserTabWidget*>(parent), this),
    m_tabPinMap()
{
    setAcceptDrops(true);
    setDocumentMode(true);
    setExpanding(false);
    setTabsClosable(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setObjectName(QLatin1String("tabBar"));
    setUsesScrollButtons(false);
    setElideMode(Qt::ElideRight);

    // Add "New Tab" button
    m_buttonNewTab = new QToolButton(this);
    const bool isDarkTheme = sBrowserApplication->isDarkTheme();
    m_buttonNewTab->setIcon(isDarkTheme ? QIcon(QStringLiteral(":/new-tab-white.png")) : QIcon(QStringLiteral(":/new-tab.png")));
    m_buttonNewTab->setStyleSheet(QLatin1String("QToolButton:hover { border: 1px solid #666666; border-radius: 2px; } "));
    m_buttonNewTab->setToolTip(tr("New Tab"));
    m_buttonNewTab->setFixedSize(28, height() - 2);

    //setStyleSheet("QTabBar::tab:disabled { background-color: rgba(0, 0, 0, 0); }");
    connect(m_buttonNewTab, &QToolButton::clicked, this, &BrowserTabBar::newTabRequest);

    // Add shortcut (Ctrl+Tab) to switch between tabs
    QShortcut *tabShortcut = new QShortcut(QKeySequence(QKeySequence::NextChild), this);
    connect(tabShortcut, &QShortcut::activated, this, &BrowserTabBar::onNextTabShortcut);

    // Custom context menu
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &BrowserTabBar::customContextMenuRequested, this, &BrowserTabBar::onContextMenuRequest);

    // Adjust tab pin state on tab move event
    connect(this, &BrowserTabBar::tabMoved, this, &BrowserTabBar::onTabMoved);

    m_tabPinMap[0] = false;
}

QToolButton *BrowserTabBar::getNewTabButton() const
{
    return m_buttonNewTab;
}

bool BrowserTabBar::isTabPinned(int tabIndex) const
{
    if (tabIndex < 0 || tabIndex >= count())
        return false;

    return m_tabPinMap.at(tabIndex);
}

void BrowserTabBar::setTabPinned(int index, bool value)
{
    if (index < 0 || index >= count() || m_tabPinMap.at(index) == value)
        return;

    m_tabPinMap[index] = value;
    forceRepaint();

    emit tabPinned(index, value);
}

void BrowserTabBar::onNextTabShortcut()
{
    int nextIdx = currentIndex() + 1;

    if (nextIdx >= count())
        setCurrentIndex(0);
    else
        setCurrentIndex(nextIdx);
}

void BrowserTabBar::onContextMenuRequest(const QPoint &pos)
{
    QMenu menu(this);

    BrowserTabWidget *tabWidget = qobject_cast<BrowserTabWidget*>(parentWidget());

    // Add "New Tab" menu item, shown on every context menu request
    menu.addAction(tr("New tab"), this, &BrowserTabBar::newTabRequest, QKeySequence(tr("Ctrl+T")));
    // Check if the user right-clicked on a tab, or just some position on the tab bar
    int tabIndex = tabAt(pos);
    if (tabIndex < 0)
    {
        if (tabWidget)
        {
            menu.addSeparator();
            QAction *reopenTabAction = menu.addAction(tr("Reopen closed tab"), tabWidget, &BrowserTabWidget::reopenLastTab);
            reopenTabAction->setEnabled(tabWidget->canReopenClosedTab());
        }
        menu.exec(mapToGlobal(pos));
        return;
    }

    menu.addSeparator();
    QAction *reloadAction = menu.addAction(tr("Reload"), this, [this, tabIndex]() {
        emit reloadTabRequest(tabIndex);
    });
    reloadAction->setShortcut(QKeySequence(tr("Ctrl+R")));

    const bool isPinned = m_tabPinMap.at(tabIndex);
    const QString pinTabText = isPinned ? tr("Unpin tab") : tr("Pin tab");
    menu.addAction(pinTabText, this, [this, tabIndex, isPinned](){
        setTabPinned(tabIndex, !isPinned);
    });

    menu.addAction(tr("Duplicate tab"), this, [this, tabIndex]() {
        emit duplicateTabRequest(tabIndex);
    });

    if (tabWidget)
    {
        if (WebWidget *ww = tabWidget->getWebWidget(tabIndex))
        {
            if (WebPage *page = ww->page())
            {
                const bool isTabMuted = page->isAudioMuted();

                const QString muteActionText = isTabMuted ? tr("Unmute tab") : tr("Mute tab");
                menu.addAction(muteActionText, this, [page, isTabMuted]() {
                    page->setAudioMuted(!isTabMuted);
                });
            }

            const QString hibernateActionText = ww->isHibernating() ? tr("Wake up tab") : tr("Hibernate tab");
            menu.addAction(hibernateActionText, this, [ww](){
                ww->setHibernation(!ww->isHibernating());
            });
        }
    }

    menu.addSeparator();
    QAction *closeAction = menu.addAction(tr("Close tab"), this, [this, tabIndex]() {
        removeTab(tabIndex);
    });
    closeAction->setShortcut(QKeySequence(tr("Ctrl+W")));

    if (tabWidget)
    {
        menu.addSeparator();
        QAction *reopenTabAction = menu.addAction(tr("Reopen closed tab"), tabWidget, &BrowserTabWidget::reopenLastTab);
        reopenTabAction->setEnabled(tabWidget->canReopenClosedTab());
    }

    menu.exec(mapToGlobal(pos));
}

void BrowserTabBar::onTabMoved(int from, int to)
{
    bool stateFrom = m_tabPinMap.at(from);
    if (from < to)
    {
        for (int i = from; i <= to; ++i)
            m_tabPinMap[i] = m_tabPinMap[i + 1];
    }
    else if (from > to)
    {
        for (int i = to + 1; i <= from; ++i)
            m_tabPinMap[i] = m_tabPinMap[i - 1];
    }
    else
        return;

    m_tabPinMap[to] = stateFrom;

    forceRepaint();
}

void BrowserTabBar::mousePressEvent(QMouseEvent *event)
{
    m_mimeDelegate.onMousePress(event);
    QTabBar::mousePressEvent(event);
}

void BrowserTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::LeftButton))
        m_mimeDelegate.onMouseMove(event);

    QTabBar::mouseMoveEvent(event);
}

void BrowserTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    if (!m_mimeDelegate.onDragEnter(event))
        QTabBar::dragEnterEvent(event);
}

void BrowserTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    m_mimeDelegate.onDragLeave(event);
    QTabBar::dragLeaveEvent(event);
    update();
}

void BrowserTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    m_mimeDelegate.onDragMove(event);
    QTabBar::dragMoveEvent(event);
}

void BrowserTabBar::dropEvent(QDropEvent *event)
{
    if (!m_mimeDelegate.onDrop(event))
        QTabBar::dropEvent(event);
}

void BrowserTabBar::paintEvent(QPaintEvent *event)
{
    QTabBar::paintEvent(event);
    m_mimeDelegate.onPaint();
}

QSize BrowserTabBar::sizeHint() const
{
    QSize hint = QTabBar::sizeHint();
    hint.setWidth(hint.width() - 2 - m_buttonNewTab->width());
    return hint;
}

void BrowserTabBar::tabLayoutChange()
{
    QTabBar::tabLayoutChange();
    moveNewTabButton();
}

void BrowserTabBar::tabInserted(int index)
{
    std::map<int, bool> tabPinMap;
    for (auto it : m_tabPinMap)
    {
        int idx = it.first;
        if (idx >= index)
            ++idx;
        tabPinMap[idx] = it.second;
    }
    tabPinMap[index] = false;
    m_tabPinMap = tabPinMap;

    QTabBar::tabInserted(index);
}

void BrowserTabBar::tabRemoved(int index)
{
    std::map<int, bool> tabPinMap;
    for (auto it : m_tabPinMap)
    {
        int idx = it.first;
        if (idx > index)
            --idx;
        tabPinMap[idx] = it.second;
    }
    m_tabPinMap = tabPinMap;

    QTabBar::tabRemoved(index);
    forceRepaint();
}

QSize BrowserTabBar::tabSizeHint(int index) const
{
    // Get the QTabBar size hint and keep width within an upper bound
    QSize hint = QTabBar::tabSizeHint(index);

    QSize pinnedTabSize(iconSize().width() * 3 + 6, hint.height());

    if (m_tabPinMap.at(index))
        return pinnedTabSize;

    const int numTabs = count();

    int numPinnedTabs = 0;
    for (auto it : m_tabPinMap)
    {
        if (it.second)
            ++numPinnedTabs;
    }

    int pinnedTabWidth = numPinnedTabs * pinnedTabSize.width();

    int tabWidth = float(geometry().width() - m_buttonNewTab->width() - pinnedTabWidth - 1) / (numTabs - numPinnedTabs);
    tabWidth = std::max(pinnedTabSize.width(), tabWidth);
    tabWidth = std::min(tabWidth, (geometry().width() - m_buttonNewTab->width() - pinnedTabWidth - 1) / 7);

    return hint.boundedTo(QSize(tabWidth, hint.height()));
}

void BrowserTabBar::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);
    moveNewTabButton();
}

void BrowserTabBar::moveNewTabButton()
{
    int numTabs = count();
    if (numTabs == 0)
        return;

    int tabWidth = 2;
    for (int i = 0; i < numTabs; ++i)
        tabWidth += tabRect(i).width();

    QRect barRect = rect();
    if (tabWidth > width())
        m_buttonNewTab->move(barRect.right() - m_buttonNewTab->width(), barRect.y()); //m_buttonNewTab->hide();
    else
    {
        m_buttonNewTab->move(barRect.left() + tabWidth, barRect.y());
        m_buttonNewTab->show();
    }
}

void BrowserTabBar::forceRepaint()
{
    // Force resize by 1px, then reset size to force a repaint
    QSize currentSize(size());
    int currentWidth = currentSize.width();

    currentSize.setWidth(currentWidth - 1);
    resize(currentSize);

    currentSize.setWidth(currentWidth + 1);
    resize(currentSize);

    update();
}
