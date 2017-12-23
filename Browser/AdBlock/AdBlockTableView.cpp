#include "AdBlockModel.h"
#include "AdBlockTableView.h"

#include <QModelIndexList>

AdBlockTableView::AdBlockTableView(QWidget *parent) :
    QTableView(parent)
{
}

void AdBlockTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    AdBlockModel *tableModel = static_cast<AdBlockModel*>(model());
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
