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
    enum TimeRangeOption { LAST_HOUR = 0, LAST_TWO_HOUR = 1, LAST_FOUR_HOUR = 2, LAST_DAY = 3, TIME_RANGE_ALL = 4 };

public:
    explicit ClearHistoryDialog(QWidget *parent = 0);
    ~ClearHistoryDialog();

    /// Returns the user-selected time range for history modification
    TimeRangeOption getTimeRange() const;

    /// Returns a bitfield containing the types of user data to be deleted
    HistoryType getHistoryTypes() const;

private slots:
    /// Toggles the details list widget display
    void toggleDetails();

private:
    /// Adds items to the list widget
    void setupListWidget();

private:
    Ui::ClearHistoryDialog *ui;
};

#endif // CLEARHISTORYDIALOG_H
