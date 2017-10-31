#ifndef COOKIETABLEVIEW_H
#define COOKIETABLEVIEW_H

#include <QTableView>

/**
 * @class CookieTableView
 * @brief Subclasses the QTableView class in order to define custom selection behavior
 *        of the checkboxes belonging to the first column of each cookie row in the table
 */
class CookieTableView : public QTableView
{
public:
    /// CookieTableView constructor
    CookieTableView(QWidget *parent = nullptr);

protected:
    /// Defines custom selection behavior for the first column of the table
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
};

#endif // COOKIETABLEVIEW_H
