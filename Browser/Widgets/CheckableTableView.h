#ifndef CHECKABLETABLEVIEW_H
#define CHECKABLETABLEVIEW_H

#include <QTableView>

/**
 * @class CheckableTableView
 * @brief Subclasses the QTableView class that defines custom selection behavior
 *        of the checkboxes belonging to the first column of each row in the view
 */
class CheckableTableView : public QTableView
{
public:
    /// Table view constructor
    CheckableTableView(QWidget *parent = nullptr);

protected:
    /// Defines custom selection behavior for the first column of the table
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
};

#endif // CHECKABLETABLEVIEW_H
