#ifndef CLEARHISTORYDIALOG_H
#define CLEARHISTORYDIALOG_H

#include <cstdint>
#include "ClearHistoryOptions.h"
#include <QDialog>

namespace Ui {
class ClearHistoryDialog;
}

/**
 * @class ClearHistoryDialog
 * @brief Dialog used to clear browsing history, cookies, cache, etc.
 */
class ClearHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    /// Options for the time range combo box
    enum TimeRangeOption { LAST_HOUR = 0, LAST_TWO_HOUR = 1, LAST_FOUR_HOUR = 2, LAST_DAY = 3, TIME_RANGE_ALL = 4, CUSTOM_RANGE = 5 };

public:
    explicit ClearHistoryDialog(QWidget *parent = 0);
    ~ClearHistoryDialog();

    /// Returns the user-selected time range for history modification
    TimeRangeOption getTimeRange() const;

    /// Returns a bitfield containing the types of user data to be deleted
    HistoryType getHistoryTypes() const;

    /// Returns a pair { start date, end date } of the user-set time range to be clear, so long as the time range option is set to CUSTOM_RANGE
    std::pair<QDateTime, QDateTime> getCustomTimeRange() const;

private Q_SLOTS:
    /// Toggles the details list widget display
    void toggleDetails();

    /// Called when a \ref TimeRangeOption has been selected from the dropdown menu
    void onTimeRangeSelected(int index);

private:
    /// Sets the visibility of the specific date-time range items in the dialog
    void setCustomRangeVisiblity(bool visible);

    /// Adds items to the list widget
    void setupListWidget();

private:
    Ui::ClearHistoryDialog *ui;
};

#endif // CLEARHISTORYDIALOG_H
