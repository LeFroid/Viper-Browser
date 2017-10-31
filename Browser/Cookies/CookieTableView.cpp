#include "CookieTableModel.h"
#include "CookieTableView.h"

#include <QModelIndexList>

CookieTableView::CookieTableView(QWidget *parent) :
    QTableView(parent)
{
}

void CookieTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    CookieTableModel *tableModel = static_cast<CookieTableModel*>(model());
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
