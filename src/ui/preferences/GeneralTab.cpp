#include "GeneralTab.h"
#include "ui_GeneralTab.h"

#include "BrowserSetting.h"

#include <QDir>

GeneralTab::GeneralTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GeneralTab)
{
    ui->setupUi(this);

    // Populate the combobox for startup mode options
    ui->comboBoxStartup->addItem(tr("Show my home page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadHomePage)));
    ui->comboBoxStartup->addItem(tr("Show a blank page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadBlankPage)));
    ui->comboBoxStartup->addItem(tr("Show the welcome page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadNewTabPage)));
    ui->comboBoxStartup->addItem(tr("Show my tabs from last time"), QVariant::fromValue(static_cast<int>(StartupMode::RestoreSession)));

    ui->lineEditDownloadDir->setFileMode(QFileDialog::Directory);

    // Disable download directory line edit when "always ask where to save files" is activated
    connect(ui->radioButtonAskForDir, &QRadioButton::clicked, this, &GeneralTab::toggleLineEditDownloadDir);
    connect(ui->radioButtonSaveToDir, &QRadioButton::clicked, this, &GeneralTab::toggleLineEditDownloadDir);
}

GeneralTab::~GeneralTab()
{
    delete ui;
}

void GeneralTab::setDownloadAskBehavior(bool alwaysAsk)
{
    ui->radioButtonSaveToDir->setChecked(!alwaysAsk);
    ui->radioButtonAskForDir->setChecked(alwaysAsk);
    ui->lineEditDownloadDir->setEnabled(!alwaysAsk);
}

bool GeneralTab::getDownloadAskBehavior() const
{
    return ui->radioButtonAskForDir->isChecked();
}

QString GeneralTab::getDownloadDirectory() const
{
    return ui->lineEditDownloadDir->text();
}

void GeneralTab::setDownloadDirectory(const QString &path)
{
    ui->lineEditDownloadDir->setText(path);
}

QString GeneralTab::getHomePage() const
{
    return ui->lineEditHomePage->text();
}

void GeneralTab::setHomePage(const QString &homePage)
{
    ui->lineEditHomePage->setText(homePage);
}

int GeneralTab::getStartupIndex() const
{
    return ui->comboBoxStartup->currentIndex();
}

void GeneralTab::setStartupIndex(int index)
{
    ui->comboBoxStartup->setCurrentIndex(index);
}

NewTabType GeneralTab::getNewTabPageType() const
{
    if (ui->radioButtonTabHomePage->isChecked())
        return NewTabType::HomePage;
    if (ui->radioButtonTabBlankPage->isChecked())
        return NewTabType::BlankPage;

    return NewTabType::FavoritesPage;
}

void GeneralTab::setNewTabPageType(NewTabType value)
{
    ui->radioButtonTabHomePage->setChecked(value == NewTabType::HomePage);
    ui->radioButtonTabBlankPage->setChecked(value == NewTabType::BlankPage);
    ui->radioButtonTabWelcomePage->setChecked(value == NewTabType::FavoritesPage);
}

bool GeneralTab::openAllTabsInBackground() const
{
    return ui->checkBoxNewTabsInBackground->isChecked();
}

void GeneralTab::setAllTabsOpenInBackground(bool value)
{
    ui->checkBoxNewTabsInBackground->setChecked(value);
}

void GeneralTab::toggleLineEditDownloadDir()
{
    ui->lineEditDownloadDir->setEnabled(!ui->lineEditDownloadDir->isEnabled());
}
