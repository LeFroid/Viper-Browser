#ifndef HISTORYWIDGET_H
#define HISTORYWIDGET_H

#include <QModelIndex>
#include <QWidget>

namespace Ui {
class HistoryWidget;
}

class HistoryManager;
class QSortFilterProxyModel;

/// Range of times used to narrow browser history shown in the table
enum class HistoryRange
{
    Week      = 0,
    Fortnight = 1,
    Month     = 2,
    Year      = 3,
    All       = 4,
    RangeMax  = 5
};

/**
 * @class HistoryWidget
 * @brief Dialog used to view the user's browsing history over a period of time
 */
class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the history widget
    explicit HistoryWidget(QWidget *parent = nullptr);

    /// Destroys the UI
    ~HistoryWidget();

    /// Sets the history manager which is used to load data into the history table
    void setHistoryManager(HistoryManager *manager);

    /// Loads all history items matching the current filter, as set in the list widget
    void loadHistory();

signals:
    /// Emitted when the user requests to open the given link in the active browser tab
    void openLink(const QUrl &url);

    /// Emitted when the user requests to open the given link in a new browser tab
    void openLinkNewTab(const QUrl &url, bool makeCurrent = false);

    /// Emitted when the user requests to open the given link in a new browser window
    void openLinkNewWindow(const QUrl &url);

protected:
    /// Called to adjust the proportions of the columns belonging to the table view
    virtual void resizeEvent(QResizeEvent *event) override;

private slots:
    /// Called when a context menu is requested within the table view
    void onContextMenuRequested(const QPoint &pos);

    /// Called when the history range selection has changed
    void onCriteriaChanged(const QModelIndex &index);

    /// Called to search the browser history for a search term contained in the line edit widget
    void searchHistory();

private:
    /// Sets the items in the history criteria list
    void setupCriteriaList();

    /// Returns a QDateTime object representing the start date to load history items from, depending on m_timeRange
    QDateTime getLoadDate();

private:
    /// UI form class
    Ui::HistoryWidget *ui;

    /// Proxy model used for searching through history
    QSortFilterProxyModel *m_proxyModel;

    /// Time range being used to view history
    HistoryRange m_timeRange;
};

#endif // HISTORYWIDGET_H
