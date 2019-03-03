#include "ExemptThirdPartyCookieDialog.h"
#include "ui_ExemptThirdPartyCookieDialog.h"

#include "CookieJar.h"

ExemptThirdPartyCookieDialog::ExemptThirdPartyCookieDialog(CookieJar *cookieJar, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ExemptThirdPartyCookieDialog),
    m_cookieJar(cookieJar),
    m_hostsToAdd(),
    m_hostsToDelete()
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);

    connect(ui->pushButtonAddHost, &QPushButton::clicked, this, &ExemptThirdPartyCookieDialog::onAddHostButtonClicked);
    connect(ui->pushButtonRemoveHost, &QPushButton::clicked, this, &ExemptThirdPartyCookieDialog::onRemoveHostButtonClicked);
    connect(ui->pushButtonRemoveAll, &QPushButton::clicked, this, &ExemptThirdPartyCookieDialog::onRemoveAllHostsButtonClicked);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &ExemptThirdPartyCookieDialog::onDialogButtonClicked);

    loadHosts();
}

ExemptThirdPartyCookieDialog::~ExemptThirdPartyCookieDialog()
{
    delete ui;
}

void ExemptThirdPartyCookieDialog::loadHosts()
{
    auto exemptHosts = m_cookieJar->getExemptThirdPartyHosts();
    if (exemptHosts.empty())
    {
        ui->pushButtonRemoveHost->setEnabled(false);
        ui->pushButtonRemoveAll->setEnabled(false);
    }

    for (const auto &host : exemptHosts)
    {
        ui->listWidgetHosts->addItem(host.toString());
    }
}

void ExemptThirdPartyCookieDialog::onAddHostButtonClicked()
{
    QUrl websiteUrl = QUrl::fromUserInput(ui->lineEditHost->text());
    if (websiteUrl.isEmpty() || !websiteUrl.isValid())
        return;

    m_hostsToAdd.insert(websiteUrl);

    ui->listWidgetHosts->addItem(websiteUrl.toString());

    ui->pushButtonRemoveHost->setEnabled(true);
    ui->pushButtonRemoveAll->setEnabled(true);
}

void ExemptThirdPartyCookieDialog::onRemoveHostButtonClicked()
{
    auto currentItem = ui->listWidgetHosts->takeItem(ui->listWidgetHosts->currentIndex().row());
    if (!currentItem)
        return;

    QUrl websiteUrl(currentItem->text());

    m_hostsToAdd.remove(websiteUrl);
    m_hostsToDelete.insert(websiteUrl);

    delete currentItem;

    if (ui->listWidgetHosts->count() == 0)
    {
        ui->pushButtonRemoveHost->setEnabled(false);
        ui->pushButtonRemoveAll->setEnabled(false);
    }
}

void ExemptThirdPartyCookieDialog::onRemoveAllHostsButtonClicked()
{
    for (int i = 0; i < ui->listWidgetHosts->count(); ++i)
    {
        auto item = ui->listWidgetHosts->item(i);
        if (!item)
            continue;

        QUrl websiteUrl(item->text());

        m_hostsToAdd.remove(websiteUrl);
        m_hostsToDelete.insert(websiteUrl);
    }

    ui->listWidgetHosts->clear();
}

void ExemptThirdPartyCookieDialog::onDialogButtonClicked(QAbstractButton *button)
{
    if (button == ui->buttonBox->button(QDialogButtonBox::Save))
    {
        for (const auto &host : m_hostsToAdd)
        {
            m_cookieJar->addThirdPartyExemption(host);
        }

        for (const auto &host : m_hostsToDelete)
        {
            m_cookieJar->removeThirdPartyExemption(host);
        }
    }

    close();
}
