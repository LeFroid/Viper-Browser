#ifndef BROWSERTABBAR_H
#define BROWSERTABBAR_H

#include "WebWidget.h"

#include <map>
#include <QTabBar>
#include <QUrl>

class QToolButton;

/// Used during BrowserTabBar's dragMoveEvent when a file is being dragged over the bar.
/// A drop indicator arrow will be rendered in one of the following areas relative
/// to the nearest tab.
enum class DropIndicatorLocation
{
    Center,
    Left,
    Right
};

/**
 * @struct ExternalDropInfo
 * @brief Stores information about an external resource that's
 *        potentially going to be dropped onto the tab bar
 */
struct ExternalDropInfo
{
    /// Location relative to the nearest tab
    DropIndicatorLocation Location;

    /// Index of the nearest tab
    int NearestTabIndex;

    /// Default constructor
    ExternalDropInfo() : Location(DropIndicatorLocation::Center), NearestTabIndex(-1) {}

    /// Constructs the object with a specific location and nearest tab index
    ExternalDropInfo(DropIndicatorLocation location, int nearestTabIndex) :
        Location(location),
        NearestTabIndex(nearestTabIndex)
    {
    }
};

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

    /// Returns true if the tab at the given index is pinned, false if else
    bool isTabPinned(int tabIndex) const;

signals:
    /// Emitted when the user chooses to duplicate the tab at the given index
    void duplicateTabRequest(int index);

    /// Emitted when the "New Tab" button is activated
    void newTabRequest();

    /// Emitted when the user chooses the "Reload" option on a tab from the context menu
    void reloadTabRequest(int index);

public slots:
    /// Sets the tab at the given index to be pinned if value is true, otherwise it will be unpinned
    void setTabPinned(int index, bool value);

private slots:
    /// Called when the next tab shortcut is activated. Switches to the next tab, or cycles back to the first tab if already at the last tab
    void onNextTabShortcut();

    /// Creates a context menu at the given position on the tab bar
    void onContextMenuRequest(const QPoint &pos);

    /// Called when a tab has moved from the index at position from, to the index at position to
    void onTabMoved(int from, int to);

protected:
    /// Called when the user presses the mouse over the tab bar. Initiates drag-and-drop events
    void mousePressEvent(QMouseEvent *event) override;

    /// Called when the mouse is moved over the tab bar. Handles drag operations.
    void mouseMoveEvent(QMouseEvent *event) override;

    /// Handles drag events
    void dragEnterEvent(QDragEnterEvent *event) override;

    /// Handles drag leave events
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    /// Handles drag move events
    void dragMoveEvent(QDragMoveEvent *event) override;

    /// Handles drop events
    void dropEvent(QDropEvent *event) override;

    /// Handles paint events when a drop indicator is involved
    void paintEvent(QPaintEvent *event) override;

    /// Returns the suggested size of the browser tab bar
    QSize sizeHint() const override;

    /// Called when the tab layout is changed
    void tabLayoutChange() override;

    /// Handler that is called after a new tab was inserted at position index
    void tabInserted(int index) override;

    /// Handler that is called after a tab was removed from position index
    void tabRemoved(int index) override;

    /// Returns a size hint of the tab with the given index
    QSize tabSizeHint(int index) const override;

    /// Resizes elements and shifts positions in the tab bar on resize events
    void resizeEvent(QResizeEvent *event) override;

private:
    /// Moves the "New Tab" button so that it is aligned to the right of the last tab in the tab bar
    void moveNewTabButton();

    /// Forces a repaint by changing the width of the tab bar by 1 pixel, then restoring to the original size
    void forceRepaint();

private:
    /// The "New Tab" button to the right of the last tab in the tab bar
    QToolButton *m_buttonNewTab;

    /// Stores the starting position of a tab drag event
    QPoint m_dragStartPos;

    /// Stores the state of the \ref WebWidget in the tab being dragged
    WebState m_draggedTabState;

    /// Flag reflecting whether or not the tab being dragged is hibernating
    bool m_isDraggedTabHibernating;

    /// Keeps the contents of the tab region in a pixmap for drag-and-drop events
    QPixmap m_dragPixmap;

    /// Information about a drag and drop event from a resource external to the browser
    ExternalDropInfo m_externalDropInfo;

    /// Map of tab indexes to their pinned/unpinned state. If the value is true, the tab is pinned.
    std::map<int, bool> m_tabPinMap;
};

#endif // BROWSERTABBAR_H
