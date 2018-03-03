#include "AdBlockSubscribeDialog.h"
#include "ui_AdBlockSubscribeDialog.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

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
    QFile f(QLatin1String(":/adblock_recommended.json"));
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;

    QByteArray subData = f.readAll();
    f.close();

    // Parse the JSON object for recommended subscription objects
    QJsonDocument subscriptionDoc(QJsonDocument::fromJson(subData));
    QJsonObject subscriptionObj = subscriptionDoc.object();
    for (auto it = subscriptionObj.begin(); it != subscriptionObj.end(); ++it)
    {
        AdBlockSubscriptionInfo subInfo;
        subInfo.Name = it.key();

        QJsonObject itemInfoObj = it.value().toObject();
        subInfo.SubscriptionURL = QUrl(itemInfoObj.value(QLatin1String("source")).toString());

        if (itemInfoObj.contains(QLatin1String("resource")))
            subInfo.ResourceURL = QUrl(itemInfoObj.value(QLatin1String("resource")).toString());

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
