#include "FolderNavigationAction.h"
#include "BookmarkNode.h"
#include "BookmarkTableModel.h"
#include "BookmarkWidget.h"

#include <QTreeView>

FolderNavigationAction::FolderNavigationAction(const QModelIndex &index,
                                               BookmarkTableModel *tableModel,
                                               BookmarkWidget *bookmarkUi,
                                               QTreeView *treeView) :
    m_index(index),
    m_tableModel(tableModel),
    m_bookmarkUi(bookmarkUi),
    m_treeView(treeView)
{
}

void FolderNavigationAction::execute()
{
    if (!m_index.isValid())
        return;

    BookmarkNode *folder = static_cast<BookmarkNode*>(m_index.internalPointer());
    m_tableModel->setCurrentFolder(folder);

    if (m_index != m_treeView->currentIndex())
        m_treeView->setCurrentIndex(m_index);

    m_bookmarkUi->showInfoForNode(folder);
}
