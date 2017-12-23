#ifndef ADBLOCKTABLEVIEW_H
#define ADBLOCKTABLEVIEW_H

#include <QTableView>

/**
 * @class UserScriptTableView
 * @brief Subclasses the QTableView class in order to define custom selection behavior
 *        of the checkboxes belonging to the first column of each row in the ad blocks table
 */
class AdBlockTableView : public QTableView
{
public:
    /// Table view constructor
    AdBlockTableView(QWidget *parent = nullptr);

protected:
    /// Defines custom selection behavior for the first column of the table
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
};

#endif // ADBLOCKTABLEVIEW_H
