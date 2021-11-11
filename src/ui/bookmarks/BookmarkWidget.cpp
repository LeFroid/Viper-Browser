#include "BookmarkWidget.h"
#include "ui_BookmarkWidget.h"
#include "BookmarkExporter.h"
#include "BookmarkFolderModel.h"
#include "BookmarkImporter.h"
#include "BookmarkTableModel.h"
#include "BookmarkNode.h"
#include "BrowserApplication.h"
#include "FolderNavigator.h"

#include <algorithm>
#include <set>
#include <vector>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QResizeEvent>
#include <QtConcurrent>
#include <QDebug>

BookmarkWidget::BookmarkWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::BookmarkWidget),
    m_bookmarkManager(nullptr),
    m_tableModel(nullptr),
    m_currentNode(nullptr),
    m_navigator(nullptr)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);

    // Setup history buttons
    const bool isDarkTheme = sBrowserApplication->isDarkTheme();
    ui->buttonBack->setIcon(QIcon(isDarkTheme ? QStringLiteral(":/arrow-back-white.png") : QStringLiteral(":/arrow-back.png")));
    ui->buttonBack->setEnabled(false);

    ui->buttonForward->setIcon(QIcon(isDarkTheme ? QStringLiteral(":/arrow-forward-white.png") : QStringLiteral(":/arrow-forward.png")));
    ui->buttonForward->setEnabled(false);

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
    connect(ui->lineEditSearch,   &QLineEdit::returnPressed, this, &BookmarkWidget::searchBookmarks);

    // Bookmark editing elements
    connect(ui->lineEditName,     &QLineEdit::returnPressed, this, &BookmarkWidget::onEditNodeName);
    connect(ui->lineEditLocation, &QLineEdit::returnPressed, this, &BookmarkWidget::onEditNodeURL);
    connect(ui->lineEditShortcut, &QLineEdit::returnPressed, this, &BookmarkWidget::onEditNodeShortcut);

    // Set proportions of splitter items
    QList<int> splitterSizes;
    const int totalWidth = width();
    splitterSizes.push_back(totalWidth / 3);
    splitterSizes.push_back(totalWidth * 2 / 3);
    ui->splitter->setSizes(splitterSizes);
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);
}

BookmarkWidget::~BookmarkWidget()
{
    delete ui;
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

    m_tableModel = new BookmarkTableModel(bookmarkManager, this);
    ui->tableView->setModel(m_tableModel);

    // Setup the navigation handler
    m_navigator = new FolderNavigator(this, m_tableModel, ui->treeView);
    connect(m_navigator, &FolderNavigator::historyChanged, this, &BookmarkWidget::onHistoryChanged);
    connect(ui->buttonBack,    &QPushButton::clicked, m_navigator, &FolderNavigator::goBack);
    connect(ui->buttonForward, &QPushButton::clicked, m_navigator, &FolderNavigator::goForward);

    // Add context menu to bookmark view
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &BookmarkWidget::onBookmarkContextMenu);
    connect(ui->tableView->selectionModel(), &QItemSelectionModel::currentChanged, this, &BookmarkWidget::onBookmarkSelectionChanged);

    connect(m_tableModel, &BookmarkTableModel::dataChanged, this, &BookmarkWidget::onTableDataChanged);

    // Connect change in folder data via table model to an update in the folder model
    connect(m_tableModel, &BookmarkTableModel::movedFolder, this, &BookmarkWidget::resetFolderModel);

    // Set up bookmark folder model and view
    BookmarkFolderModel *treeViewModel = new BookmarkFolderModel(bookmarkManager, this);
    ui->treeView->setModel(treeViewModel);
    ui->treeView->header()->hide();

    // Add context menu to folder view (for adding or removing folders)
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &BookmarkWidget::onFolderContextMenu);
    setupFolderModel(treeViewModel);
}

void BookmarkWidget::onBookmarkContextMenu(const QPoint &pos)
{
    QModelIndex index = ui->tableView->indexAt(pos);
    bool isBookmarkNode = false;

    QMenu menu(this);
    if (index.isValid())
    {
        // Check if URL column is empty. If so, display context menu for a folder node instead of the bookmark
        // context menu
        QModelIndex urlIdx = m_tableModel->index(index.row(), 1, index.parent());
        isBookmarkNode = !m_tableModel->data(urlIdx).isNull();
    }

    if (isBookmarkNode) // Display context menu items for bookmark nodes
    {
        // Add action to open bookmark, and make the font bold
        QAction *openAction = menu.addAction(tr("Open"), this, &BookmarkWidget::openInCurrentPage);
        QFont openFont = openAction->font();
        openFont.setBold(true);
        openAction->setFont(openFont);

        menu.addAction(tr("Open in a new tab"),    this, &BookmarkWidget::openInNewTab);
        menu.addAction(tr("Open in a new window"), this, &BookmarkWidget::openInNewWindow);
        menu.addSeparator();
    }
    else               // Display context menu items for folder nodes
    {
        BookmarkNode *currentFolder = m_tableModel->getCurrentFolder();
        if (currentFolder != nullptr)
        {
            int nodeIdx = index.row();
            BookmarkNode *subFolder = currentFolder->getNode(nodeIdx);
            menu.addAction(tr("Open all items in tabs"), this, [this, subFolder](){
                openAllBookmarksNewTabs(subFolder);
            });
            menu.addSeparator();
        }
    }

    if (!m_tableModel->isInSearchMode())
    {
        menu.addAction(tr("New bookmark..."), this, &BookmarkWidget::addBookmark);
        menu.addAction(tr("New folder..."),   this, &BookmarkWidget::addFolder);
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
            menu.addAction(tr("Open all items in tabs"), this, [this, f](){
                openAllBookmarksNewTabs(f);
            });
            menu.addSeparator();
        }

        menu.addAction(tr("New bookmark..."), this, &BookmarkWidget::addBookmark);
        menu.addAction(tr("New folder..."),   this, &BookmarkWidget::addFolder);
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, &BookmarkWidget::deleteFolderSelection);
    }
    else
    {
        menu.addAction(tr("New folder..."), this, &BookmarkWidget::addTopLevelFolder);
    }

    menu.exec(mapToGlobal(pos));
}

void BookmarkWidget::onBookmarkSelectionChanged(const QModelIndex &current, const QModelIndex &/*previous*/)
{
    if (!current.isValid())
        return;

    // Get node and update line edit widgets
    if (BookmarkNode *node = m_tableModel->getBookmark(current.row()))
        showInfoForNode(node);
    else
        m_currentNode = nullptr;
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
            BookmarkNode *importFolder = m_bookmarkManager->addFolder(tr("Imported Bookmarks"), m_bookmarkManager->getRoot());
            resetFolderModel();

            QtConcurrent::run([this, fileName, importFolder](){
                BookmarkImporter importer(m_bookmarkManager);
                if (!importer.import(fileName, importFolder))
                    qDebug() << "Error: In BookmarkWidget, could not import bookmarks from file " << fileName;

                resetFolderModel();
            });

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
    Q_EMIT openBookmark(getUrlForSelection());
}

void BookmarkWidget::openInNewTab()
{
    Q_EMIT openBookmarkNewTab(getUrlForSelection());
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
            Q_EMIT openBookmarkNewTab(child->getURL());
    }
}

void BookmarkWidget::openInNewWindow()
{
    Q_EMIT openBookmarkNewWindow(getUrlForSelection());
}

void BookmarkWidget::addBookmark()
{
    m_tableModel->insertRow(m_tableModel->rowCount());
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
    QModelIndexList items = ui->tableView->selectionModel()->selectedIndexes();

    // Selection mode is by rows, so remove duplicate items if multiple columns of same row are selected
    std::set<int> rowSet;
    for (auto index : items)
        rowSet.insert(index.row());

    // Sort rows in descending order
    std::vector<int> rows;
    for (int r : rowSet)
        rows.push_back(r);
    std::sort(rows.begin(), rows.end(), std::greater<int>());

    for (int r : rows)
        m_tableModel->removeRows(r, 1);
}

void BookmarkWidget::deleteFolderSelection()
{
    // Make sure the table model is not using the any of the folder selections
    m_tableModel->setCurrentFolder(m_bookmarkManager->getBookmarksBar());

    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    QModelIndexList items = ui->treeView->selectionModel()->selectedIndexes();
    ui->treeView->setCurrentIndex(model->index(0, 0));
    for (auto index : items)
        model->removeRow(index.row(), index.parent());
}

void BookmarkWidget::searchBookmarks()
{
    const QString term = ui->lineEditSearch->text();
    m_tableModel->searchFor(term);

    showInfoForNode(nullptr);
}

void BookmarkWidget::resetFolderModel()
{
    QAbstractItemModel *oldModel = ui->treeView->model();
    QItemSelectionModel *oldSelectionModel = ui->treeView->selectionModel();
    disconnect(oldModel, &QAbstractItemModel::dataChanged, this, &BookmarkWidget::onFolderDataChanged);
    disconnect(oldSelectionModel, &QItemSelectionModel::currentChanged, m_navigator, &FolderNavigator::onFolderActivated);

    BookmarkFolderModel *updatedModel = new BookmarkFolderModel(m_bookmarkManager, this);
    ui->treeView->setModel(updatedModel);
    ui->treeView->setExpanded(updatedModel->index(0, 0), true);
    oldModel->deleteLater();
    oldSelectionModel->deleteLater();

    showInfoForNode(nullptr);
    setupFolderModel(updatedModel);
}

void BookmarkWidget::setupFolderModel(BookmarkFolderModel *folderModel)
{
    // Clear history items
    clearNavigationHistory();

    // Auto select root folder and expand it
    QModelIndex root = folderModel->index(0, 0);
    ui->treeView->setCurrentIndex(root);
    ui->treeView->setExpanded(root, true);

    m_navigator->initialize(root);

    // Folder model selection and data handlers
    connect(ui->treeView->selectionModel(), &QItemSelectionModel::currentChanged, m_navigator, &FolderNavigator::onFolderActivated);
    connect(folderModel,  &BookmarkFolderModel::dataChanged, this, &BookmarkWidget::onFolderDataChanged);

    // Connect changes in bookmark data via folder model to an update in the table model
    connect(folderModel, &BookmarkFolderModel::beginMovingBookmarks, this, &BookmarkWidget::beginResetTableModel);
    connect(folderModel, &BookmarkFolderModel::endMovingBookmarks,   this, &BookmarkWidget::endResetTableModel);
    connect(folderModel, &BookmarkFolderModel::movedFolder,          this, &BookmarkWidget::onFolderMoved);
}

void BookmarkWidget::beginResetTableModel()
{
    m_tableModel->beginResetModel();
}

void BookmarkWidget::endResetTableModel()
{
    // Tell the table model to load the bookmarks bar folder to avoid null pointer references
    m_tableModel->setCurrentFolder(m_tableModel->getCurrentFolder());
    m_tableModel->endResetModel();

    showInfoForNode(nullptr);

    // Clear history items
    clearNavigationHistory();
}

void BookmarkWidget::onFolderMoved(BookmarkNode *folder, BookmarkNode *updatedPtr)
{
    // Check if the folder was being displayed in the table, and if so, update the pointer
    if (m_tableModel->getCurrentFolder() == folder)
        m_tableModel->setCurrentFolder(updatedPtr);

    // Clear history items
    clearNavigationHistory();
}

void BookmarkWidget::onFolderDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &/*roles*/)
{
    if (!m_currentNode || m_currentNode->getType() != BookmarkNode::Folder || topLeft != bottomRight)
        return;

    BookmarkFolderModel *model = static_cast<BookmarkFolderModel*>(ui->treeView->model());
    if (m_currentNode == model->getItem(topLeft))
        showInfoForNode(m_currentNode);
}

void BookmarkWidget::onTableDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &/*roles*/)
{
    if (!m_currentNode)
        return;

    for (int row = topLeft.row(); row <= bottomRight.row(); ++row)
    {
        if (m_currentNode == m_tableModel->getBookmark(row))
        {
            showInfoForNode(m_currentNode);
            return;
        }
    }
}

void BookmarkWidget::onEditNodeName()
{
    if (!m_currentNode)
        return;

    QString newName = ui->lineEditName->text();
    if (newName.isEmpty())
        return;

    if (m_currentNode->getType() == BookmarkNode::Bookmark)
    {
        // Edit through the table model
        int numRows = m_tableModel->rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            if (m_currentNode == m_tableModel->getBookmark(i))
            {
                static_cast<void>(m_tableModel->setData(m_tableModel->index(i, 0), newName, Qt::EditRole));
                return;
            }
        }
    }
    else if (m_currentNode->getType() == BookmarkNode::Folder)
    {
        // Have to update both models in this case
        BookmarkFolderModel *folderModel = static_cast<BookmarkFolderModel*>(ui->treeView->model());

        bool foundIndex = false;
        QModelIndex folderIndex;

        std::deque<QModelIndex> indices;
        indices.push_back(QModelIndex());
        while (!indices.empty())
        {
            QModelIndex index = indices.front();
            if (m_currentNode == folderModel->getItem(index))
            {
                foundIndex = true;
                folderIndex = index;
                break;
            }

            for (int i = 0; i < folderModel->rowCount(index); ++i)
            {
                QModelIndex child = folderModel->index(i, 0, index);
                indices.push_back(child);
            }
            indices.pop_front();
        }

        bool isFolderInTableModel = false;
        int numRows = m_tableModel->rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            if (m_currentNode == m_tableModel->getBookmark(i))
            {
                isFolderInTableModel = true;
                break;
            }
        }

        if (isFolderInTableModel)
            m_tableModel->beginResetModel();

        if (foundIndex)
            static_cast<void>(folderModel->setData(folderIndex, newName, Qt::EditRole));
        else
            qDebug() << "Couldn't find folder index";

        if (isFolderInTableModel)
        {
            m_tableModel->endResetModel();
            m_tableModel->setCurrentFolder(m_tableModel->getCurrentFolder());
        }
    }
}

void BookmarkWidget::onEditNodeURL()
{
    if (!m_currentNode || m_currentNode->getType() != BookmarkNode::Bookmark)
        return;

    QString newUrl = ui->lineEditLocation->text();
    if (newUrl.isEmpty())
        return;

    // Edit URL through the table model
    int numRows = m_tableModel->rowCount();
    for (int i = 0; i < numRows; ++i)
    {
        if (m_currentNode == m_tableModel->getBookmark(i))
        {
            static_cast<void>(m_tableModel->setData(m_tableModel->index(i, 1), newUrl, Qt::EditRole));
            return;
        }
    }
}

void BookmarkWidget::onEditNodeShortcut()
{
    if (!m_currentNode)
        return;

    m_bookmarkManager->setBookmarkShortcut(m_currentNode, ui->lineEditShortcut->text());
}

void BookmarkWidget::onHistoryChanged()
{
    ui->buttonBack->setEnabled(m_navigator->canGoBack());
    ui->buttonForward->setEnabled(m_navigator->canGoForward());
}

void BookmarkWidget::clearNavigationHistory()
{
    m_navigator->clear();
    ui->buttonBack->setEnabled(false);
    ui->buttonForward->setEnabled(false);
}

QUrl BookmarkWidget::getUrlForSelection()
{
    int row = ui->tableView->currentIndex().row();
    QString link = m_tableModel->data(m_tableModel->index(row, 1)).toString();
    return QUrl::fromUserInput(link);
}

void BookmarkWidget::showInfoForNode(BookmarkNode *node)
{
    m_currentNode = node;

    if (node == nullptr)
    {
        ui->lineEditName->setText(QString());
        ui->lineEditLocation->setText(QString());
        ui->lineEditShortcut->setText(QString());
        return;
    }

    ui->lineEditName->setText(node->getName());

    if (node->getType() == BookmarkNode::Bookmark)
    {
        ui->labelSelectedLocation->show();
        ui->lineEditLocation->setText(node->getURL().toString(QUrl::FullyEncoded));
        ui->lineEditLocation->show();

        ui->labelSelectedShortcut->show();
        ui->lineEditShortcut->setText(node->getShortcut());
        ui->lineEditShortcut->show();
    }
    else
    {
        ui->labelSelectedLocation->hide();
        ui->lineEditLocation->hide();

        ui->labelSelectedShortcut->hide();
        ui->lineEditShortcut->hide();
    }
}
