#include "BookmarkWidget.h"
#include "ui_bookmarkwidget.h"
#include "BookmarkExporter.h"
#include "BookmarkFolderModel.h"
#include "BookmarkImporter.h"
#include "BookmarkTableModel.h"

#include <algorithm>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QRegExp>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QDebug>

BookmarkWidget::BookmarkWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BookmarkWidget),
    m_bookmarkManager(nullptr),
    m_proxyModel(new QSortFilterProxyModel(this))
{
    ui->setupUi(this);

    ui->buttonBack->setIcon(style()->standardIcon(QStyle::SP_ArrowBack, 0, this));
    ui->buttonForward->setIcon(style()->standardIcon(QStyle::SP_ArrowForward, 0, this));

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

    // Set properties of proxy model used for bookmark searches
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1); // -1 applies search terms to all columns

    // Enable search for bookmarks
    connect(ui->lineEditSearch, &QLineEdit::editingFinished, this, &BookmarkWidget::searchBookmarks);
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

void BookmarkWidget::setBookmarkManager(std::shared_ptr<BookmarkManager> bookmarkManager)
{
    m_bookmarkManager = bookmarkManager;

    BookmarkTableModel *tableModel = new BookmarkTableModel(bookmarkManager, this);
    m_proxyModel->setSourceModel(tableModel);
    ui->tableView->setModel(m_proxyModel);

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

    // Add context menu to folder view (for adding or removing folders)
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &BookmarkWidget::onFolderContextMenu);

    // Connect change in selection in tree view to a change in data display in table view
    connect(ui->treeView, &QTreeView::clicked, this, &BookmarkWidget::onChangeFolderSelection);
}

void BookmarkWidget::onBookmarkContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->tableView->indexAt(pos);

    QMenu menu(this);
    if (index.isValid())
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

    menu.addAction(tr("New bookmark..."), this, &BookmarkWidget::addBookmark);
    menu.addAction(tr("New folder..."), this, &BookmarkWidget::addFolder);

    if (index.isValid())
    {
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &BookmarkWidget::deleteBookmarkSelection);
    }

    menu.exec(ui->tableView->mapToGlobal(pos));
}

void BookmarkWidget::onFolderContextMenu(const QPoint &pos)
{
    //TODO: Implement "Open all items in tabs" action
    QModelIndex index = ui->treeView->indexAt(pos);
    if (!index.isValid())
        return;

    BookmarkFolder *f = static_cast<BookmarkFolder*>(index.internalPointer());
    QMenu menu(this);

    // Allow user to open all bookmark items if there are > 0 within the folder
    if (!f->bookmarks.empty())
    {
        menu.addAction(tr("Open all items in tabs"));
        menu.addSeparator();
    }

    menu.addAction(tr("New bookmark..."), this, &BookmarkWidget::addBookmark);
    menu.addAction(tr("New folder..."), this, &BookmarkWidget::addFolder);

    // Allow deletion of folder if not root bookmark folder
    if (f->id > 0)
    {
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &BookmarkWidget::deleteFolderSelection);
    }
    menu.exec(mapToGlobal(pos));
}

void BookmarkWidget::onChangeFolderSelection(const QModelIndex &index)
{
    // Get the pointer to the new folder and update the tabel model
    BookmarkFolder *f = static_cast<BookmarkFolder*>(index.internalPointer());
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(m_proxyModel->sourceModel());
    model->setCurrentFolder(f);
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
            BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
            QModelIndex rootIndex = model->index(0, 0);

            int importFolderIdx = model->rowCount(rootIndex);
            model->insertRow(importFolderIdx, rootIndex);
            QModelIndex importedIndex = model->index(importFolderIdx, 0, rootIndex);
            model->setData(importedIndex, QString("Imported Bookmarks"), Qt::EditRole);

            BookmarkFolder *importFolder = static_cast<BookmarkFolder*>(importedIndex.internalPointer());
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

void BookmarkWidget::openInNewWindow()
{
    emit openBookmarkNewWindow(getUrlForSelection());
}

void BookmarkWidget::addBookmark()
{
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(m_proxyModel->sourceModel());
    model->insertRow(model->rowCount());
}

void BookmarkWidget::addFolder()
{
    //TODO: allow user to modify the position of folders
    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    model->insertRow(0, ui->treeView->currentIndex());
}

void BookmarkWidget::deleteBookmarkSelection()
{
    BookmarkTableModel *model = static_cast<BookmarkTableModel*>(m_proxyModel->sourceModel());
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
    BookmarkTableModel *tableModel = static_cast<BookmarkTableModel*>(m_proxyModel->sourceModel());
    tableModel->setCurrentFolder(m_bookmarkManager->getRoot());

    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    QModelIndexList items = ui->treeView->selectionModel()->selectedIndexes();
    for (auto index : items)
        model->removeRow(index.row(), index.parent());
}

void BookmarkWidget::searchBookmarks()
{
    m_proxyModel->setFilterRegExp(ui->lineEditSearch->text());
}

QUrl BookmarkWidget::getUrlForSelection()
{
    int row = ui->tableView->currentIndex().row();
    QString link = m_proxyModel->data(m_proxyModel->index(row, 1)).toString();
    return QUrl::fromUserInput(link);
}
