#include "UserScriptModel.h"
#include "UserScriptTableView.h"

#include <QModelIndexList>

UserScriptTableView::UserScriptTableView(QWidget *parent) :
    QTableView(parent)
{
}

void UserScriptTableView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    UserScriptModel *tableModel = static_cast<UserScriptModel*>(model());
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
