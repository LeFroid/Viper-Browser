#include "BrowserTabBar.h"

//TODO: Context menu for tabBar

BrowserTabBar::BrowserTabBar(QWidget *parent) :
    QTabBar(parent)
{
    setExpanding(false);
    setTabsClosable(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setElideMode(Qt::ElideRight);
}

QSize BrowserTabBar::tabSizeHint(int index) const
{
    // Get the QTabBar size hint and keep width within an upper bound
    QSize hint = QTabBar::tabSizeHint(index);
    QFontMetrics fMetric = fontMetrics();
    return hint.boundedTo(QSize(fMetric.width("R") * 20, hint.height()));
}
