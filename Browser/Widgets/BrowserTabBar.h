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

    /// Emitted when the user chooses the "Reload" option on a tab from the context menu
    void reloadTabRequest(int index);

private slots:
    /// Called when the next tab shortcut is activated. Switches to the next tab, or cycles back to the first tab if already at the last tab
    void onNextTabShortcut();

    /// Creates a context menu at the given position on the tab bar
    void onContextMenuRequest(const QPoint &pos);

protected:
    /// Returns the suggested size of the browser tab bar
    QSize sizeHint() const override;

    /// Called when the tab layout is changed
    void tabLayoutChange() override;

    /// Returns a size hint of the tab with the given index
    QSize tabSizeHint(int index) const override;

    /// Resizes elements and shifts positions in the tab bar on resize events
    void resizeEvent(QResizeEvent *event) override;

private:
    /// Moves the "New Tab" button so that it is aligned to the right of the last tab in the tab bar
    void moveNewTabButton();

private:
    /// The "New Tab" button to the right of the last tab in the tab bar
    QToolButton *m_buttonNewTab;
};

#endif // BROWSERTABBAR_H
