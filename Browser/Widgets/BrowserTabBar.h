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

private slots:
    /// Called when the next tab shortcut is activated. Switches to the next tab, or cycles back to the first tab if already at the last tab
    void onNextTabShortcut();

protected:
    /// Returns a size hint of the tab with the given index
    QSize tabSizeHint(int index) const override;

    /// Handles mouse move events, ensuring that a tab isn't moved to the right of the "New tab" pseudo tab
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    /// The "New Tab" button to the right of the last tab in the tab bar
    QToolButton *m_buttonNewTab;
};

#endif // BROWSERTABBAR_H
