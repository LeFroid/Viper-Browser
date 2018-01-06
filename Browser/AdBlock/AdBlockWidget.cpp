#include "AdBlockWidget.h"
#include "ui_AdBlockWidget.h"
#include "AdBlockManager.h"
#include "AdBlockModel.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QUrl>

AdBlockWidget::AdBlockWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AdBlockWidget)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

    ui->tableView->setModel(AdBlockManager::instance().getModel());

    connect(ui->tableView, &CheckableTableView::clicked, this, &AdBlockWidget::onItemClicked);
    connect(ui->pushButtonAddSubscription, &QPushButton::clicked, this, &AdBlockWidget::onAddSubscriptionButtonClicked);
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

void AdBlockWidget::onAddSubscriptionButtonClicked()
{
    bool ok;
    QString userInput = QInputDialog::getText(this, tr("Install Subscription"), tr("Enter the URL of the subscription:"),
                                              QLineEdit::Normal, QString(), &ok);
    if (!ok || userInput.isEmpty())
        return;

    QUrl subUrl = QUrl::fromUserInput(userInput);
    if (!subUrl.isValid())
    {
        static_cast<void>(QMessageBox::warning(this, tr("Installation Error"), tr("Could not install subscription."), QMessageBox::Ok,
                                               QMessageBox::Ok));
        return;
    }
    AdBlockManager::instance().installSubscription(subUrl);
}
