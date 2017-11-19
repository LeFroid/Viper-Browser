#include "BrowserTabBar.h"

#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
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
    m_buttonNewTab->setToolTip(tr("New Tab"));
    //m_buttonNewTab->setFixedHeight(height() - 2);
    m_buttonNewTab->setFixedSize(28, height() - 2);
    /*int addTabIdx = addTab(QString());
    setTabButton(addTabIdx, QTabBar::RightSide, m_buttonNewTab);
    setTabToolTip(addTabIdx, tr("New Tab"));
    setTabEnabled(addTabIdx, false);*/

    setStyleSheet("QTabBar::tab:disabled { background-color: rgba(0, 0, 0, 0); }");
    connect(m_buttonNewTab, &QToolButton::clicked, this, &BrowserTabBar::newTabRequest);

    // Add shortcut (Ctrl+Tab) to switch between tabs
    QShortcut *tabShortcut = new QShortcut(QKeySequence(QKeySequence::NextChild), this);
    connect(tabShortcut, &QShortcut::activated, this, &BrowserTabBar::onNextTabShortcut);
}

void BrowserTabBar::onNextTabShortcut()
{
    int nextIdx = currentIndex() + 1;

    if (nextIdx >= count())
        setCurrentIndex(0);
    else
        setCurrentIndex(nextIdx);
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

QSize BrowserTabBar::tabSizeHint(int index) const
{
    // Get the QTabBar size hint and keep width within an upper bound
    QSize hint = QTabBar::tabSizeHint(index);
    if (count() > 3)
    {
        QFontMetrics fMetric = fontMetrics();
        return hint.boundedTo(QSize(fMetric.width("R") * 20, hint.height()));
    }
    return hint;
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
        m_buttonNewTab->hide();//move(barRect.right() - m_buttonNewTab->width(), barRect.y());
    else
    {
        m_buttonNewTab->move(barRect.left() + tabWidth, barRect.y());
        m_buttonNewTab->show();
    }
}
