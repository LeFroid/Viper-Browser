#ifndef USERSCRIPTTABLEVIEW_H
#define USERSCRIPTTABLEVIEW_H

#include <QTableView>

/**
 * @class UserScriptTableView
 * @brief Subclasses the QTableView class in order to define custom selection behavior
 *        of the checkboxes belonging to the first column of each row in the user scripts table
 */
class UserScriptTableView : public QTableView
{
public:
    /// Table view constructor
    UserScriptTableView(QWidget *parent = nullptr);

protected:
    /// Defines custom selection behavior for the first column of the table
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
};

#endif // USERSCRIPTTABLEVIEW_H
