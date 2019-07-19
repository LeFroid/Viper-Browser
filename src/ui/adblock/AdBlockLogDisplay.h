#ifndef ADBLOCKLOGDISPLAY_H
#define ADBLOCKLOGDISPLAY_H

#include <QUrl>
#include <QWidget>

namespace Ui {
    class AdBlockLogDisplay;
}

namespace adblock {
    class AdBlockManager;
    class LogTableModel;
}
class QSortFilterProxyModel;

/**
 * @class AdBlockLogDisplay
 * @brief Provides a detailed log of the network activity that was affected by the ad block system
 */
class AdBlockLogDisplay : public QWidget
{
    Q_OBJECT

    enum LogSource { LogSourcePageUrl = 0, LogSourceAll = 1 };

public:
    /// Constructs the log display with a given parent
    explicit AdBlockLogDisplay(adblock::AdBlockManager *adBlockManager);

    /// Log display destructor
    ~AdBlockLogDisplay();

    /// Sets the table data to contain the logs that are associated with the given URL
    void setLogTableFor(const QUrl &url);

    /// Shows all log entries in the ad block system
    void showAllLogs();

private Q_SLOTS:
    /// Searches and filters the log table for the string contained in the line edit
    void onSearchTermEntered();

    /// Reloads the log data
    void onReloadClicked();

    /// Maps the selection in the combo box to the appropriate log source filter
    void onComboBoxIndexChanged(int index);

private:
    /// Pointer to the UI elements
    Ui::AdBlockLogDisplay *ui;

    /// Pointer to the advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

    /// Proxy model used to manipulate the data shown in the log table
    QSortFilterProxyModel *m_proxyModel;

    /// Source model behind the proxy
    adblock::LogTableModel *m_sourceModel;

    /// Source of the log data - currently this is either for a specific URL or for the entire ad block system
    LogSource m_logSource;

    /// First party URL from which the logs are taken. Only used when the log source type is LogSourcePageUrl
    QUrl m_sourceUrl;
};

#endif // ADBLOCKLOGDISPLAY_H
