#include "CheckableTableView.h"

CheckableTableView::CheckableTableView(QWidget *parent) :
    QTableView(parent)
{
}

void CheckableTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QAbstractItemModel *tableModel = model();
    QModelIndexList indexListSelected = selected.indexes();
    QVariant emptyVal;
    for (auto idx : indexListSelected)
    {
        // If column 0, check or uncheck the item
        if (idx.column() == 0)
            tableModel->setData(idx, emptyVal, Qt::CheckStateRole);
    }

    QTableView::selectionChanged(selected, deselected);
}
