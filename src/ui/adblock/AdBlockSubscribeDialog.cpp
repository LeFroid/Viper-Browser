#include "AdBlockSubscribeDialog.h"
#include "ui_AdBlockSubscribeDialog.h"

#include "RecommendedSubscriptions.h"

AdBlockSubscribeDialog::AdBlockSubscribeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AdBlockSubscribeDialog),
    m_subscriptions()
{
    ui->setupUi(this);

    load();
}

AdBlockSubscribeDialog::~AdBlockSubscribeDialog()
{
    delete ui;
}

std::vector<AdBlockSubscriptionInfo> AdBlockSubscribeDialog::getSubscriptions() const
{
    std::vector<AdBlockSubscriptionInfo> selection;

    int numItems = ui->listWidgetSubscriptions->count();
    for (int i = 0; i < numItems; ++i)
    {
        QListWidgetItem *item = ui->listWidgetSubscriptions->item(i);
        if (item == nullptr)
            continue;

        if (item->checkState() == Qt::Checked)
            selection.push_back(m_subscriptions.at(item->data(Qt::UserRole).toInt()));
    }

    return selection;
}

void AdBlockSubscribeDialog::load()
{
    adblock::RecommendedSubscriptions recommendedSubs;
    if (recommendedSubs.empty())
        return;

    for (const std::pair<QString, QUrl> &item : recommendedSubs)
    {
        AdBlockSubscriptionInfo subInfo;
        subInfo.Name = item.first;
        subInfo.SubscriptionURL = item.second;
        m_subscriptions.push_back(subInfo);
    }

    // Iterate through subscription container, adding each item to the list widget
    int numSubscriptions = static_cast<int>(m_subscriptions.size());
    for (int i = 0; i < numSubscriptions; ++i)
    {
        const AdBlockSubscriptionInfo &item = m_subscriptions.at(i);
        QListWidgetItem* listItem = new QListWidgetItem(item.Name, ui->listWidgetSubscriptions);

        // Make each item checkable
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(Qt::Unchecked);

        // Set user data to contain index of subscription item in container
        listItem->setData(Qt::UserRole, i);
    }
}
