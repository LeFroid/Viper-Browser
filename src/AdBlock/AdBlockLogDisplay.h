#ifndef ADBLOCKLOGDISPLAY_H
#define ADBLOCKLOGDISPLAY_H

#include <QUrl>
#include <QWidget>

namespace Ui {
    class AdBlockLogDisplay;
}

class AdBlockLogTableModel;
class QSortFilterProxyModel;

/**
 * @class AdBlockLogDisplay
 * @brief Provides a detailed log of the network activity that was affected by the ad block system
 */
class AdBlockLogDisplay : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the log display with a given parent
    explicit AdBlockLogDisplay(QWidget *parent = nullptr);

    /// Log display destructor
    ~AdBlockLogDisplay();

    /// Sets the table data to contain the logs that are associated with the given URL
    void setLogTableFor(const QUrl &url);

private slots:
    /// Searches and filters the log table for the string contained in the line edit
    void onSearchTermEntered();

private:
    /// Pointer to the UI elements
    Ui::AdBlockLogDisplay *ui;

    /// Proxy model used to manipulate the data shown in the log table
    QSortFilterProxyModel *m_proxyModel;

    /// Source model behind the proxy
    AdBlockLogTableModel *m_sourceModel;
};

#endif // ADBLOCKLOGDISPLAY_H
