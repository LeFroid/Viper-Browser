#include "BookmarkWidget.h"
#include "ui_bookmarkwidget.h"
#include "BookmarkExporter.h"
#include "BookmarkFolderModel.h"
#include "BookmarkImporter.h"
#include "BookmarkTableModel.h"
#include "BookmarkNode.h"

#include <algorithm>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QRegExp>
#include <QResizeEvent>
#include <QDebug>

BookmarkWidget::BookmarkWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BookmarkWidget),
    m_bookmarkManager(nullptr),
    m_folderBackHistory(),
    m_folderForwardHistory()
{
    ui->setupUi(this);

    // Setup history buttons
    ui->buttonBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    ui->buttonBack->setEnabled(false);
    connect(ui->buttonBack, &QPushButton::clicked, this, &BookmarkWidget::onClickBackButton);

    ui->buttonForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));
    ui->buttonForward->setEnabled(false);
    connect(ui->buttonForward, &QPushButton::clicked, this, &BookmarkWidget::onClickForwardButton);

    // Setup combo box items for importing / exporting bookmarks
    ui->comboBoxOptions->addItem(tr("Import or Export"),
                                 static_cast<int>(ComboBoxOption::NoAction));
    ui->comboBoxOptions->addItem(tr("Import bookmarks from HTML"),
                                 static_cast<int>(ComboBoxOption::ImportHTML));
    ui->comboBoxOptions->addItem(tr("Export bookmarks to HTML"),
                                 static_cast<int>(ComboBoxOption::ExportHTML));

    connect(ui->comboBoxOptions, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &BookmarkWidget::onImportExportBoxChanged);

    // Drag and drop in bookmark table
    ui->tableView->setDropIndicatorShown(true);
    ui->tableView->setDragDropOverwriteMode(false);

    // Enable search for bookmarks
    connect(ui->lineEditSearch, &QLineEdit::textChanged, this, &BookmarkWidget::searchBookmarks);
}

BookmarkWidget::~BookmarkWidget()
{
    delete ui;
}

void BookmarkWidget::closeEvent(QCloseEvent *event)
{
    emit managerClosed();
    QWidget::closeEvent(event);
}

void BookmarkWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int tableWidth = ui->tableView->geometry().width();
    ui->tableView->setColumnWidth(0, std::max(tableWidth / 3, 0));
    ui->tableView->setColumnWidth(1, std::max(tableWidth * 2 / 3 - 3, 0));
}

void BookmarkWidget::setBookmarkManager(BookmarkManager *bookmarkManager)
{
    m_bookmarkManager = bookmarkManager;

    BookmarkTableModel *tableModel = new BookmarkTableModel(bookmarkManager, this);
    ui->tableView->setModel(tableModel);

    // Add context menu to bookmark view
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &BookmarkWidget::onBookmarkContextMenu);

    // Set up bookmark folder model and view
    BookmarkFolderModel *treeViewModel = new BookmarkFolderModel(bookmarkManager, this);
    ui->treeView->setModel(treeViewModel);
    ui->treeView->header()->hide();

    // Auto select root folder and expand it
    QModelIndex root = treeViewModel->index(0, 0);
    ui->treeView->setCurrentIndex(root);
    ui->treeView->setExpanded(root, true);
    m_folderBackHistory.push_back(root);

    // Add context menu to folder view (for adding or removing folders)
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &BookmarkWidget::onFolderContextMenu);

    // Connect change in selection in tree view to a change in data display in table view
    connect(ui->treeView, &QTreeView::clicked, this, &BookmarkWidget::onChangeFolderSelection);

    // Connect change in folder data via table model to an update in the folder model
    connect(tableModel, &BookmarkTableModel::movedFolder, this, &BookmarkWidget::resetFolderModel);

    // Connect changes in bookmark data via folder model to an update in the table model
    connect(treeViewModel, &BookmarkFolderModel::beginMovingBookmarks, this, &BookmarkWidget::beginResetTableModel);
    connect(treeViewModel, &BookmarkFolderModel::endMovingBookmarks,   this, &BookmarkWidget::endResetTableModel);
    connect(treeViewModel, &BookmarkFolderModel::movedFolder,          this, &BookmarkWidget::onFolderMoved);
}

void BookmarkWidget::reloadBookmarks()
{
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    model->setCurrentFolder(m_bookmarkManager->getRoot());

    BookmarkFolderModel *folderModel =  static_cast<BookmarkFolderModel*>(ui->treeView->model());
    folderModel->dataChanged(folderModel->index(0, 0),
                             folderModel->index(folderModel->rowCount() - 1, 0));
}

void BookmarkWidget::onBookmarkContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->tableView->indexAt(pos);
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    bool isBookmarkNode = false;

    QMenu menu(this);
    if (index.isValid())
    {
        // Check if URL column is empty. If so, display context menu for a folder node instead of the bookmark
        // context menu
        QModelIndex urlIdx = model->index(index.row(), 1, index.parent());
        isBookmarkNode = !model->data(urlIdx).isNull();
    }

    if (isBookmarkNode) // Display context menu items for bookmark nodes
    {
        // Add action to open bookmark, and make the font bold
        QAction *openAction = menu.addAction(tr("Open"), this, &BookmarkWidget::openInCurrentPage);
        QFont openFont = openAction->font();
        openFont.setBold(true);
        openAction->setFont(openFont);

        menu.addAction(tr("Open in a new tab"), this, &BookmarkWidget::openInNewTab);
        menu.addAction(tr("Open in a new window"), this, &BookmarkWidget::openInNewWindow);
        menu.addSeparator();
    }
    else               // Display context menu items for folder nodes
    {
        BookmarkNode *currentFolder = model->getCurrentFolder();
        if (currentFolder != nullptr)
        {
            int nodeIdx = index.row();
            BookmarkNode *subFolder = currentFolder->getNode(nodeIdx);
            menu.addAction(tr("Open all items in tabs"), [=](){ openAllBookmarksNewTabs(subFolder); });
            menu.addSeparator();
        }
    }

    if (!model->isInSearchMode())
    {
        menu.addAction(tr("New bookmark..."), this, &BookmarkWidget::addBookmark);
        menu.addAction(tr("New folder..."), this, &BookmarkWidget::addFolder);
    }

    if (isBookmarkNode)
    {
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &BookmarkWidget::deleteBookmarkSelection);
    }

    menu.exec(ui->tableView->mapToGlobal(pos));
}

void BookmarkWidget::onFolderContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->treeView->indexAt(pos);
    QMenu menu(this);

    if (index.isValid())
    {
        BookmarkNode *f = static_cast<BookmarkNode*>(index.internalPointer());

        // Allow user to open all bookmark items if there are > 0 within the folder
        if (f->getNumChildren() > 0)
        {
            menu.addAction(tr("Open all items in tabs"), [=](){ openAllBookmarksNewTabs(f); });
            menu.addSeparator();
        }

        menu.addAction(tr("New bookmark..."), this, &BookmarkWidget::addBookmark);
        menu.addAction(tr("New folder..."), this, &BookmarkWidget::addFolder);
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &BookmarkWidget::deleteFolderSelection);
    }
    else
    {
        menu.addAction(tr("New folder..."), this, &BookmarkWidget::addTopLevelFolder);
    }

    menu.exec(mapToGlobal(pos));
}

void BookmarkWidget::onChangeFolderSelection(const QModelIndex &index)
{
    // Store the currently selected index at the front of the history queue
    if (m_folderBackHistory.size() < 2)
        ui->buttonBack->setEnabled(true);
    m_folderForwardHistory.clear();
    ui->buttonForward->setEnabled(false);

    // Get the pointer to the new folder and update the tabel model
    BookmarkNode *f = static_cast<BookmarkNode*>(index.internalPointer());
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    model->setCurrentFolder(f);

    m_folderBackHistory.push_back(ui->treeView->currentIndex());
}

void BookmarkWidget::onImportExportBoxChanged(int index)
{
    if (index < 0)
        return;

    switch (static_cast<ComboBoxOption>(ui->comboBoxOptions->currentData(Qt::UserRole).toInt()))
    {
        case ComboBoxOption::ImportHTML:
        {
            QString fileName = QFileDialog::getOpenFileName(this, tr("Import Bookmark File"), QDir::homePath(),
                                                            QString("HTML File(*.html *.htm)"));
            if (fileName.isNull())
                return;

            // Create an "Imported Bookmarks" folder
            BookmarkNode *importFolder = m_bookmarkManager->addFolder("Imported Bookmarks", m_bookmarkManager->getRoot());
            resetFolderModel();

            BookmarkImporter importer(m_bookmarkManager);
            if (!importer.import(fileName, importFolder))
                qDebug() << "Error: In BookmarkWidget, could not import bookmarks from file " << fileName;

            break;
        }
        case ComboBoxOption::ExportHTML:
        {
            QString fileName = QFileDialog::getSaveFileName(this, tr("Export Bookmark File"),
                                                            QDir::homePath() + QDir::separator() + "bookmarks.html",
                                                            QString("HTML File(*.html)"));
            if (fileName.isNull())
                return;

            BookmarkExporter exporter(m_bookmarkManager);
            if (!exporter.saveTo(fileName))
                qDebug() << "Error: In BookmarkWidget, could not export bookmarks to file " << fileName;

            return;
        }
        case ComboBoxOption::NoAction:
        default:
            return;
    }
}

void BookmarkWidget::openInCurrentPage()
{
    emit openBookmark(getUrlForSelection());
}

void BookmarkWidget::openInNewTab()
{
    emit openBookmarkNewTab(getUrlForSelection());
}

void BookmarkWidget::openAllBookmarksNewTabs(BookmarkNode *folder)
{
    if (!folder)
        return;

    int numChildren = folder->getNumChildren();
    for (int i = 0; i < numChildren; ++i)
    {
        BookmarkNode *child = folder->getNode(i);
        if (child->getType() == BookmarkNode::Bookmark)
            emit openBookmarkNewTab(child->getURL());
    }
}

void BookmarkWidget::openInNewWindow()
{
    emit openBookmarkNewWindow(getUrlForSelection());
}

void BookmarkWidget::addBookmark()
{
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    model->insertRow(model->rowCount());
}

void BookmarkWidget::addFolder()
{
    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    model->insertRow(0, ui->treeView->currentIndex());
}

void BookmarkWidget::addTopLevelFolder()
{
    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    model->insertRow(model->rowCount());
}

void BookmarkWidget::deleteBookmarkSelection()
{
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    QModelIndexList items = ui->tableView->selectionModel()->selectedIndexes();

    // Selection mode is by rows, so remove duplicate items
    QSet<int> rows;
    for (auto index : items)
        rows << index.row();

    for (auto r : rows)
        model->removeRow(r);
}

void BookmarkWidget::deleteFolderSelection()
{
    // Make sure the table model is not using the any of the folder selections
    BookmarkTableModel *tableModel = static_cast<BookmarkTableModel*>(ui->tableView->model());
    tableModel->setCurrentFolder(m_bookmarkManager->getBookmarksBar());

    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    QModelIndexList items = ui->treeView->selectionModel()->selectedIndexes();
    ui->treeView->setCurrentIndex(model->index(0, 0));
    for (auto index : items)
        model->removeRow(index.row(), index.parent());

    if (m_folderBackHistory.empty())
        ui->buttonBack->setEnabled(false);
    if (m_folderForwardHistory.empty())
        ui->buttonForward->setEnabled(false);
}

void BookmarkWidget::searchBookmarks(const QString &term)
{
    BookmarkTableModel *tableModel = static_cast<BookmarkTableModel*>(ui->tableView->model());
    tableModel->searchFor(term);
}

void BookmarkWidget::resetFolderModel()
{
    QAbstractItemModel *oldModel = ui->treeView->model();
    BookmarkFolderModel *updatedModel = new BookmarkFolderModel(m_bookmarkManager, this);
    ui->treeView->setModel(updatedModel);
    oldModel->deleteLater();
    ui->treeView->setExpanded(updatedModel->index(0, 0), true);

    // Clear history items
    m_folderBackHistory.clear();
    m_folderForwardHistory.clear();
    ui->buttonBack->setEnabled(false);
    ui->buttonForward->setEnabled(false);
}

void BookmarkWidget::beginResetTableModel()
{
    BookmarkTableModel *tableModel = static_cast<BookmarkTableModel*>(ui->tableView->model());
    tableModel->beginResetModel();
}

void BookmarkWidget::endResetTableModel()
{
    // Tell the table model to load the bookmarks bar folder to avoid null pointer references
    BookmarkTableModel *tableModel = static_cast<BookmarkTableModel*>(ui->tableView->model());
    tableModel->endResetModel();
    tableModel->setCurrentFolder(tableModel->getCurrentFolder());

    // Clear history items
    m_folderBackHistory.clear();
    m_folderForwardHistory.clear();
    ui->buttonBack->setEnabled(false);
    ui->buttonForward->setEnabled(false);
}

void BookmarkWidget::onFolderMoved(BookmarkNode *folder, BookmarkNode *updatedPtr)
{
    // Check if the folder was being displayed in the table, and if so, update the pointer
    BookmarkTableModel *tableModel = static_cast<BookmarkTableModel*>(ui->tableView->model());
    if (tableModel->getCurrentFolder() == folder)
        tableModel->setCurrentFolder(updatedPtr);

    // Clear history items
    m_folderBackHistory.clear();
    m_folderForwardHistory.clear();
    ui->buttonBack->setEnabled(false);
    ui->buttonForward->setEnabled(false);
}

void BookmarkWidget::onClickBackButton()
{
    if (m_folderBackHistory.empty())
        return;

    // Store current index in the forward history queue
    if (m_folderForwardHistory.empty())
        ui->buttonForward->setEnabled(true);
    m_folderForwardHistory.push_back(ui->treeView->currentIndex());

    QModelIndex idx = m_folderBackHistory.back();
    if (!idx.isValid() || (idx == ui->treeView->currentIndex() && m_folderBackHistory.size() > 1))
    {
        m_folderBackHistory.pop_back();
        idx = m_folderBackHistory.back();
    }

    // Get the pointer to the folder and update the tabel model
    BookmarkNode *f = static_cast<BookmarkNode*>(idx.internalPointer());
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    model->setCurrentFolder(f);

    ui->treeView->setCurrentIndex(idx);
    m_folderBackHistory.pop_back();

    if (m_folderBackHistory.empty())
        ui->buttonBack->setEnabled(false);
}

void BookmarkWidget::onClickForwardButton()
{
    if (m_folderForwardHistory.empty())
        return;

    // Store current index in the backwards history queue
    if (m_folderBackHistory.empty())
        ui->buttonBack->setEnabled(true);
    m_folderBackHistory.push_back(ui->treeView->currentIndex());

    const QModelIndex &idx = m_folderForwardHistory.back();
    ui->treeView->setCurrentIndex(idx);

    // Get the pointer to the folder and update the tabel model
    BookmarkNode *f = static_cast<BookmarkNode*>(idx.internalPointer());
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    model->setCurrentFolder(f);

    m_folderForwardHistory.pop_back();

    if (m_folderForwardHistory.empty())
        ui->buttonForward->setEnabled(false);
}

QUrl BookmarkWidget::getUrlForSelection()
{
    int row = ui->tableView->currentIndex().row();
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(ui->tableView->model());
    QString link = model->data(model->index(row, 1)).toString();
    return QUrl::fromUserInput(link);
}
