#ifndef BROWSERTABBAR_H
#define BROWSERTABBAR_H

#include <QTabBar>

class QToolButton;

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

signals:
    /// Emitted when the "New Tab" button is activated
    void newTabRequest();

protected:
    /// Returns a size hint of the tab with the given index
    QSize tabSizeHint(int index) const override;

private:
    /// The "New Tab" button to the right of the last tab in the tab bar
    QToolButton *m_buttonNewTab;
};

#endif // BROWSERTABBAR_H
