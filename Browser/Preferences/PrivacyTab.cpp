#include "PrivacyTab.h"
#include "ui_PrivacyTab.h"

PrivacyTab::PrivacyTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrivacyTab)
{
    ui->setupUi(this);

    ui->comboBoxHistoryOptions->addItem(tr("Remember browsing history"), QVariant::fromValue(HistoryStoragePolicy::Remember));
    ui->comboBoxHistoryOptions->addItem(tr("Clear history at exit"), QVariant::fromValue(HistoryStoragePolicy::SessionOnly));
    ui->comboBoxHistoryOptions->addItem(tr("Never remember history"), QVariant::fromValue(HistoryStoragePolicy::Never));

    connect(ui->checkBoxBlockThirdParties, &QCheckBox::toggled, [=](bool checked) {
        ui->pushButtonThirdPartyExceptions->setEnabled(checked);
    });
}

PrivacyTab::~PrivacyTab()
{
    delete ui;
}

HistoryStoragePolicy PrivacyTab::getHistoryStoragePolicy() const
{
    return ui->comboBoxHistoryOptions->currentData().value<HistoryStoragePolicy>();
}

bool PrivacyTab::areCookiesEnabled() const
{
    return ui->checkBoxAllowCookies->isChecked();
}

bool PrivacyTab::areCookiesDeletedWithSession() const
{
    return ui->checkBoxRemoveCookiesOnExit->isChecked();
}

bool PrivacyTab::areThirdPartyCookiesEnabled() const
{
    return !ui->checkBoxBlockThirdParties->isChecked();
}

void PrivacyTab::setHistoryStoragePolicy(HistoryStoragePolicy policy)
{
    for (int i = 0; i < ui->comboBoxHistoryOptions->count(); ++i)
    {
        if (ui->comboBoxHistoryOptions->itemData(i).value<HistoryStoragePolicy>() == policy)
        {
            ui->comboBoxHistoryOptions->setCurrentIndex(i);
            return;
        }
    }
}

void PrivacyTab::setCookiesEnabled(bool value)
{
    ui->checkBoxAllowCookies->setChecked(value);
}

void PrivacyTab::setCookiesDeleteWithSession(bool value)
{
    ui->checkBoxRemoveCookiesOnExit->setChecked(value);
}

void PrivacyTab::setThirdPartyCookiesEnabled(bool value)
{
    ui->checkBoxBlockThirdParties->setChecked(!value);
    ui->pushButtonThirdPartyExceptions->setEnabled(!value);
}
