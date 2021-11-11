#include "HistoryWidget.h"
#include "ui_HistoryWidget.h"
#include "HistoryManager.h"
#include "HistoryTableModel.h"

#include <algorithm>
#include <QDateTime>
#include <QList>
#include <QMenu>
#include <QResizeEvent>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

HistoryWidget::HistoryWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HistoryWidget),
    m_proxyModel(new QSortFilterProxyModel(this)),
    m_timeRange(HistoryRange::Day)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);

    // Set properties of proxy model used for searches
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1); // -1 applies search terms to all columns

    // Enable search for history
    connect(ui->lineEditSearch, &QLineEdit::editingFinished, this, &HistoryWidget::searchHistory);

    // Set up criteria list
    setupCriteriaList();
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    HistoryTableModel *tableModel = new HistoryTableModel(serviceLocator, this);
    m_proxyModel->setSourceModel(tableModel);
    ui->tableView->setModel(m_proxyModel);

    // Model will emit the signals in a separate thread, force a queued connection to upadte the UI without resizing the window or hiding/showing the window
    connect(tableModel, &HistoryTableModel::rowsAboutToBeInserted, m_proxyModel, &QSortFilterProxyModel::rowsAboutToBeInserted, Qt::QueuedConnection);
    connect(tableModel, &HistoryTableModel::rowsInserted, m_proxyModel, &QSortFilterProxyModel::rowsInserted, Qt::QueuedConnection);

    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this, &HistoryWidget::onContextMenuRequested);
}

void HistoryWidget::loadHistory()
{
    static_cast<HistoryTableModel*>(m_proxyModel->sourceModel())->loadFromDate(getLoadDate());
}

void HistoryWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    adjustTableColumnSizes();
}

void HistoryWidget::onContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = ui->tableView->indexAt(pos);
    if (!index.isValid())
        return;

    // Get the URL at the row of the index for menu actions
    QModelIndex urlIndex = m_proxyModel->index(index.row(), 1, index.parent());
    QUrl url = QUrl(m_proxyModel->data(urlIndex, Qt::DisplayRole).toString());

    QMenu menu(this);
    menu.addAction(tr("Open"), this, [this, url](){
        emit openLink(url);
    });
    menu.addAction(tr("Open in a new tab"), this, [this, url](){
        emit openLinkNewTab(url);
    });
    menu.addAction(tr("Open in a new window"), this, [this, url](){
        emit openLinkNewWindow(url);
    });
    menu.exec(ui->tableView->mapToGlobal(pos));
}

void HistoryWidget::onCriteriaChanged(const QModelIndex &index)
{
    int indexData = index.data(Qt::UserRole).toInt();
    if (indexData >= static_cast<int>(HistoryRange::RangeMax) || indexData < 0)
        return;

    HistoryRange newRange = static_cast<HistoryRange>(indexData);
    if (m_timeRange != newRange)
    {
        m_timeRange = newRange;
        loadHistory();
        adjustTableColumnSizes();
    }
}

void HistoryWidget::searchHistory()
{
    m_proxyModel->setFilterFixedString(ui->lineEditSearch->text());
}

void HistoryWidget::setupCriteriaList()
{
    QStringList listItems;
    listItems << "Today"
              << "Last 7 days"
              << "Last 14 days"
              << "Last month"
              << "Last year"
              << "All";
    ui->listWidgetCriteria->addItems(listItems);

    // Associate user data type (HistoryTypes) with widget items
    QListWidgetItem *item = ui->listWidgetCriteria->item(0);
    item->setData(Qt::UserRole, static_cast<int>(HistoryRange::Day));
    item = ui->listWidgetCriteria->item(1);
    item->setData(Qt::UserRole, static_cast<int>(HistoryRange::Week));
    item = ui->listWidgetCriteria->item(2);
    item->setData(Qt::UserRole, static_cast<int>(HistoryRange::Fortnight));
    item = ui->listWidgetCriteria->item(3);
    item->setData(Qt::UserRole, static_cast<int>(HistoryRange::Month));
    item = ui->listWidgetCriteria->item(4);
    item->setData(Qt::UserRole, static_cast<int>(HistoryRange::Year));
    item = ui->listWidgetCriteria->item(5);
    item->setData(Qt::UserRole, static_cast<int>(HistoryRange::All));

    ui->listWidgetCriteria->setCurrentRow(0);

    connect(ui->listWidgetCriteria, &QListWidget::clicked, this, &HistoryWidget::onCriteriaChanged);
}

QDateTime HistoryWidget::getLoadDate()
{
    QDateTime retDate = QDateTime::currentDateTime();
    qint64 timeSubtract = 0;
    switch (m_timeRange)
    {
        case HistoryRange::Day: return QDateTime(retDate.date(), QTime(0, 0));
        case HistoryRange::Week: timeSubtract = -7; break;
        case HistoryRange::Fortnight: timeSubtract = -14; break;
        case HistoryRange::Month: timeSubtract = -31; break;
        case HistoryRange::Year: timeSubtract = -365; break;
        case HistoryRange::All: timeSubtract = -6000; break;
        default: break;
    }
    return retDate.addDays(timeSubtract);
}

void HistoryWidget::adjustTableColumnSizes()
{
    int tableWidth = ui->tableView->geometry().width();
    ui->tableView->setColumnWidth(0, std::max(tableWidth / 4, 0));
    ui->tableView->setColumnWidth(1, std::max(tableWidth / 2 - 5, 0));
    ui->tableView->setColumnWidth(2, std::max(tableWidth / 4, 0));
}
