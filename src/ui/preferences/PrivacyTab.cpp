#include "CookieJar.h"
#include "ExemptThirdPartyCookieDialog.h"
#include "PrivacyTab.h"
#include "ui_PrivacyTab.h"

//todo: add controls for AutoFill settings
PrivacyTab::PrivacyTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PrivacyTab),
    m_cookieJar(nullptr)
{
    ui->setupUi(this);

    ui->comboBoxHistoryOptions->addItem(tr("Remember browsing history"), QVariant::fromValue(static_cast<int>(HistoryStoragePolicy::Remember)));
    ui->comboBoxHistoryOptions->addItem(tr("Clear history at exit"), QVariant::fromValue(static_cast<int>(HistoryStoragePolicy::SessionOnly)));
    ui->comboBoxHistoryOptions->addItem(tr("Never remember history"), QVariant::fromValue(static_cast<int>(HistoryStoragePolicy::Never)));

    connect(ui->pushButtonViewCredentials,    &QPushButton::clicked, this, &PrivacyTab::viewSavedCredentialsRequested);
    connect(ui->pushButtonAutoFillExceptions, &QPushButton::clicked, this, &PrivacyTab::viewAutoFillExceptionsRequested);

    connect(ui->checkBoxBlockThirdParties, &QCheckBox::toggled, [=](bool checked) {
        ui->pushButtonThirdPartyExceptions->setEnabled(checked);
    });

    connect(ui->pushButtonClearHistory, &QPushButton::clicked, this, &PrivacyTab::clearHistoryRequested);
    connect(ui->pushButtonViewHistory,  &QPushButton::clicked, this, &PrivacyTab::viewHistoryRequested);

    connect(ui->pushButtonThirdPartyExceptions, &QPushButton::clicked, this, &PrivacyTab::onManageCookieExceptionsClicked);
}

PrivacyTab::~PrivacyTab()
{
    delete ui;
}

void PrivacyTab::setCookieJar(CookieJar *cookieJar)
{
    m_cookieJar = cookieJar;
}

HistoryStoragePolicy PrivacyTab::getHistoryStoragePolicy() const
{
    return static_cast<HistoryStoragePolicy>(ui->comboBoxHistoryOptions->currentData().toInt());
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

bool PrivacyTab::isAutoFillEnabled() const
{
    return ui->checkBoxEnableAutoFill->isChecked();
}

bool PrivacyTab::isDoNotTrackEnabled() const
{
    return ui->checkBoxDoNotTrack->isChecked();
}

void PrivacyTab::setAutoFillEnabled(bool value)
{
    ui->checkBoxEnableAutoFill->setChecked(value);
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

void PrivacyTab::setDoNotTrackEnabled(bool value)
{
    ui->checkBoxDoNotTrack->setChecked(value);
}

void PrivacyTab::onManageCookieExceptionsClicked()
{
    ExemptThirdPartyCookieDialog *dialog = new ExemptThirdPartyCookieDialog(m_cookieJar);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
