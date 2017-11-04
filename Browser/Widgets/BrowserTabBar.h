#ifndef BROWSERTABBAR_H
#define BROWSERTABBAR_H

#include <QTabBar>

/**
 * @class BrowserTabBar
 * @brief The tab bar shown on every window, a part of the \ref BrowserTabWidget
 */
class BrowserTabBar : public QTabBar
{
    Q_OBJECT
public:
    /// Constructs the tab bar with a given parent widget
    explicit BrowserTabBar(QWidget *parent = nullptr);

protected:
    /// Returns a size hint of the tab with the given index
    QSize tabSizeHint(int index) const override;
};

#endif // BROWSERTABBAR_H
