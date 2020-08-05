#ifndef TABBARMIMEDELEGATE_H
#define TABBARMIMEDELEGATE_H

#include <QPixmap>
#include <QPoint>
#include <QPointer>

class BrowserTabBar;
class BrowserTabWidget;
class MainWindow;
class WebWidget;

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;
class QPaintEvent;

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
 * @class TabBarMimeDelegate
 * @brief Responsible for MIME-based drag and drop events on the
 *        \ref BrowserTabBar, including browser tabs, files and URLs
 */
class TabBarMimeDelegate
{
public:
    /**
     * @brief Constructs the tab bar MIME delegate with its requisite dependencies
     * @param window Pointer to the window that contains the tab bar
     * @param tabWidget Pointer to the widget that contains the tab bar
     * @param tabBar Pointer to the tab bar
     */
    explicit TabBarMimeDelegate(MainWindow *window, BrowserTabWidget *tabWidget, BrowserTabBar *tabBar);

    /**
     * @brief Determines whether a drag enter event should be accepted by the tab bar
     * @param dragEvent Drag event in question
     * @return True if the event was accepted, false if otherwise
     */
    bool onDragEnter(QDragEnterEvent *dragEvent);

    /**
     * @brief Resets any saved data about a drag event
     * @param dragEvent The drag leave event
     */
    void onDragLeave(QDragLeaveEvent *dragEvent);

    /**
     * @brief Handles the drag move event, updating the drop indicator position and nearest
     *        tab index if there were any URLs in the MIME data
     * @param dragEvent The drag event containing MIME data and other information such as position
     */
    void onDragMove(QDragMoveEvent *dragEvent);

    /**
     * @brief Handles the completion of a drag-and-drop event containing MIME data that the
     *        tab bar can use.
     * @param dropEvent The event containing the MIME data
     * @return True if the delegate handled the drop event, otherwise returns false.
     */
    bool onDrop(QDropEvent *dropEvent);

    /**
     * @brief Determines if a drag-and-drop event might be initiated, storing information about the
     *        future event if so.
     * @param mouseEvent The mouse event that was triggered in the tab bar.
     */
    void onMousePress(QMouseEvent *mouseEvent);

    /**
     * @brief Initiates a drag event, if the requisite conditions were met in the last mouse press event
     * @param mouseEvent The mouse event that was triggered in the tab bar.
     */
    void onMouseMove(QMouseEvent *mouseEvent);

    /**
     * @brief onPaint Paints a drop indicator arrow, if there is currently a drag event involving URL MIME data
     */
    void onPaint();

private:
    /// Window containing the tab bar
    MainWindow *m_window;

    /// Widget containing the tab bar and its associated web widgets
    BrowserTabWidget *m_tabWidget;

    /// Tab bar being managed by this delegate for drag-and-drop operations
    BrowserTabBar *m_tabBar;

    /// Visual region of a tab being dragged, stored as a pixmap.
    /// When the tab is dragged outside of the bounds of the tab bar, this
    /// pixmap is rendered over the window.
    QPixmap m_dragPixmap;

    /// Point (relative to the tab bar) where a drag event was initiated
    QPoint m_dragStartPoint;

    /// Points to the web widget that is associated with a drag-and-drop event
    QPointer<WebWidget> m_draggedWebWidget;

    /// Information about a drag and drop event from a resource external to the browser
    ExternalDropInfo m_externalDropInfo;
};

#endif // TABBARMIMEDELEGATE_H
