#include "BrowserTabBar.h"

#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QShortcut>
#include <QToolButton>

//TODO: Context menu for tabBar

BrowserTabBar::BrowserTabBar(QWidget *parent) :
    QTabBar(parent)
{
    setExpanding(false);
    setTabsClosable(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setElideMode(Qt::ElideRight);

    // Add "New Tab" button
    m_buttonNewTab = new QToolButton(this);
    m_buttonNewTab->setIcon(QIcon::fromTheme("folder-new"));
    m_buttonNewTab->setStyleSheet("QToolButton:hover { border: 1px solid #666666; border-radius: 2px; } ");
    int addTabIdx = addTab(QString());
    setTabButton(addTabIdx, QTabBar::RightSide, m_buttonNewTab);
    setTabToolTip(addTabIdx, tr("New Tab"));
    setTabEnabled(addTabIdx, false);

    setStyleSheet("QTabBar::tab:disabled { background-color: rgba(0, 0, 0, 0); }");
    connect(m_buttonNewTab, &QToolButton::clicked, this, &BrowserTabBar::newTabRequest);

    // Add shortcut (Ctrl+Tab) to switch between tabs
    QShortcut *tabShortcut = new QShortcut(QKeySequence(QKeySequence::NextChild), this);
    connect(tabShortcut, &QShortcut::activated, this, &BrowserTabBar::onNextTabShortcut);
}

void BrowserTabBar::onNextTabShortcut()
{
    int nextIdx = currentIndex() + 1;
    int numTabs = count();

    if (numTabs == 1)
        return;

    if (nextIdx >= count())
        setCurrentIndex(0);
    else
        setCurrentIndex(nextIdx);
}

QSize BrowserTabBar::tabSizeHint(int index) const
{
    // Special size for "New Tab" pseudo tab
    if (index + 1 == count())
    {
        QSize newTabHint = m_buttonNewTab->sizeHint();
        newTabHint.setWidth(newTabHint.width() * 3 / 2);
        return newTabHint;
    }

    // Get the QTabBar size hint and keep width within an upper bound
    QSize hint = QTabBar::tabSizeHint(index);
    if (count() > 3)
    {
        QFontMetrics fMetric = fontMetrics();
        return hint.boundedTo(QSize(fMetric.width("R") * 20, hint.height()));
    }
    return hint;
}

void BrowserTabBar::mouseMoveEvent(QMouseEvent *event)
{
    // Do not move tab further if it is being moved towards the "New Tab" pseudo tab
    int xPos = event->pos().x();
    int index = tabAt(event->pos());
    if (index + 2 == count()
            && xPos + tabSizeHint(index).width() >= m_buttonNewTab->frameGeometry().x())
        return;

    QTabBar::mouseMoveEvent(event);
}
