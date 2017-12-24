#include "AdBlockWidget.h"
#include "ui_AdBlockWidget.h"
#include "AdBlockManager.h"
#include "AdBlockModel.h"

#include <QResizeEvent>

AdBlockWidget::AdBlockWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AdBlockWidget)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

    ui->tableView->setModel(AdBlockManager::instance().getModel());

    connect(ui->tableView, &CheckableTableView::clicked, this, &AdBlockWidget::onItemClicked);
}

AdBlockWidget::~AdBlockWidget()
{
    delete ui;
}

void AdBlockWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int tableWidth = ui->tableView->geometry().width();
    ui->tableView->setColumnWidth(0, tableWidth / 25);  // On/Off
    ui->tableView->setColumnWidth(1, tableWidth / 4);   // Name
    ui->tableView->setColumnWidth(2, tableWidth / 5);   // Last update
    ui->tableView->setColumnWidth(3, tableWidth / 2);   // Source of subscription
}

void AdBlockWidget::onItemClicked(const QModelIndex &/*index*/)
{
    ui->pushButtonEditSubscription->setEnabled(true);
    ui->pushButtonDeleteSubscription->setEnabled(true);
}
