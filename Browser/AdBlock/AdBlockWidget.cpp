#include "AdBlockWidget.h"
#include "ui_AdBlockWidget.h"
#include "AdBlockManager.h"
#include "AdBlockModel.h"
#include "AdBlockSubscribeDialog.h"
#include "CustomFilterEditor.h"

#include <QInputDialog>
#include <QMenu>
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

    // Setup "Add Subscription" menu
    QMenu *addMenu = new QMenu(ui->toolButtonAddSubscription);
    addMenu->addAction(tr("Select from list"), this, &AdBlockWidget::addSubscriptionFromList);
    addMenu->addAction(tr("Install by URL"), this, &AdBlockWidget::addSubscriptionFromURL);
    ui->toolButtonAddSubscription->setMenu(addMenu);

    connect(ui->pushButtonCustomFilters, &QPushButton::clicked, this, &AdBlockWidget::editUserFilters);
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
    ui->pushButtonDeleteSubscription->setEnabled(true);
}

void AdBlockWidget::addSubscriptionFromList()
{
    AdBlockSubscribeDialog dialog(this);
    if (dialog.exec() == QDialog::Rejected)
        return;

    // Iterate through selected subscriptions, installing each via AdBlockManager
    AdBlockManager &adBlockMgr = AdBlockManager::instance();
    std::vector<AdBlockSubscriptionInfo> subscriptions = dialog.getSubscriptions();
    for (const AdBlockSubscriptionInfo &sub : subscriptions)
    {
        if (!sub.ResourceURL.isEmpty())
            adBlockMgr.installResource(sub.ResourceURL);

        adBlockMgr.installSubscription(sub.SubscriptionURL);
    }
}

void AdBlockWidget::addSubscriptionFromURL()
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

void AdBlockWidget::editUserFilters()
{
    CustomFilterEditor *editor = new CustomFilterEditor;
    connect(editor, &CustomFilterEditor::createUserSubscription, [=](){
        AdBlockManager::instance().createUserSubscription();
    });
    connect(editor, &CustomFilterEditor::filtersModified, [=](){
        AdBlockManager::instance().reloadSubscriptions();
    });
    editor->show();
}
