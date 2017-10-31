#include "ContentTab.h"
#include "ui_ContentTab.h"

ContentTab::ContentTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContentTab)
{
    ui->setupUi(this);
}

ContentTab::~ContentTab()
{
    delete ui;
}

bool ContentTab::arePopupsEnabled() const
{
    return !ui->checkBoxBlockPopups->isChecked();
}

bool ContentTab::isAutoLoadImagesEnabled() const
{
    return ui->checkBoxAutoLoadImages->isChecked();
}

QString ContentTab::getStandardFont() const
{
    return ui->fontComboBox->currentFont().family();
}

void ContentTab::setStandardFont(const QString &fontFamily)
{
    ui->fontComboBox->setFont(QFont(fontFamily));
}

int ContentTab::getStandardFontSize() const
{
    return ui->spinBox->value();
}

void ContentTab::setStandardFontSize(int size)
{
    if (size > 0)
        ui->spinBox->setValue(size);
}

bool ContentTab::isJavaScriptEnabled() const
{
    return ui->checkBoxEnableJavaScript->isChecked();
}

void ContentTab::toggleAutoLoadImages(bool value)
{
    ui->checkBoxAutoLoadImages->setChecked(value);
}

void ContentTab::togglePopupBlock(bool value)
{
    ui->checkBoxBlockPopups->setChecked(value);
}

void ContentTab::toggleJavaScript(bool value)
{
    ui->checkBoxEnableJavaScript->setChecked(value);
}
