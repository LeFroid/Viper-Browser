#include "FolderNavigator.h"
#include "BookmarkTableModel.h"
#include "BookmarkWidget.h"

#include <iterator>
#include <QTreeView>

FolderNavigator::FolderNavigator(BookmarkWidget *parentUi, BookmarkTableModel *tableModel, QTreeView *folderTree) :
    QObject(parentUi),
    m_indexHead(0),
    m_inProgress(false),
    m_history(),
    m_bookmarkUi(parentUi),
    m_tableModel(tableModel),
    m_treeView(folderTree)
{
}

bool FolderNavigator::canGoBack() const
{
    return m_indexHead > 0;
}

bool FolderNavigator::canGoForward() const
{
    return m_indexHead + 1 < m_history.size();
}

void FolderNavigator::clear()
{
    m_indexHead = 0;
    m_history.clear();
}

void FolderNavigator::initialize(const QModelIndex &root)
{
    if (!root.isValid())
        return;

    clear();
    m_history.push_back(FolderNavigationAction(root, m_tableModel, m_bookmarkUi, m_treeView));
}

void FolderNavigator::goBack()
{
    if (!canGoBack())
        return;

    executeAt(m_indexHead - 1);
}

void FolderNavigator::goForward()
{
    if (!canGoForward())
        return;

    executeAt(m_indexHead + 1);
}

void FolderNavigator::executeAt(std::size_t index)
{
    m_inProgress = true;

    auto it = m_history.begin();
    std::advance(it, index);

    if (it != m_history.end())
    {
        it->execute();
        m_indexHead = index;
        Q_EMIT historyChanged();
    }

    m_inProgress = false;
}

void FolderNavigator::onFolderActivated(const QModelIndex &current, const QModelIndex &/*previous*/)
{
    if (m_inProgress || !current.isValid())
        return;

    // Clear forward history, then navigate
    while (canGoForward())
        m_history.pop_back();

    FolderNavigationAction command(current, m_tableModel, m_bookmarkUi, m_treeView);
    command.execute();
    m_history.push_back(std::move(command));
    m_indexHead = m_history.size() - 1;

    Q_EMIT historyChanged();
}
