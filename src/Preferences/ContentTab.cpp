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

bool ContentTab::arePluginsEnabled() const
{
    return ui->checkBoxEnablePlugins->isChecked();
}

bool ContentTab::arePopupsEnabled() const
{
    return !ui->checkBoxBlockPopups->isChecked();
}

bool ContentTab::areUserScriptsEnabled() const
{
    return ui->checkBoxEnableUserScripts->isChecked();
}

bool ContentTab::isAdBlockEnabled() const
{
    return ui->checkBoxEnableAdBlock->isChecked();
}

bool ContentTab::isAnimatedScrollEnabled() const
{
    return ui->checkBoxEnableAnimatedScroll->isChecked();
}

bool ContentTab::isAutoLoadImagesEnabled() const
{
    return ui->checkBoxAutoLoadImages->isChecked();
}

QString ContentTab::getDefaultFont() const
{
    return ui->fontComboBoxDefault->currentFont().family();
}

void ContentTab::setDefaultFont(const QString &fontFamily)
{
    QFont f(fontFamily);
    ui->fontComboBoxDefault->setFont(f);
    ui->fontComboBoxDefault->setCurrentFont(f);
}

QString ContentTab::getSerifFont() const
{
    return ui->fontComboBoxSerif->currentFont().family();
}

void ContentTab::setSerifFont(const QString &fontFamily)
{
    QFont f(fontFamily);
    ui->fontComboBoxSerif->setFont(f);
    ui->fontComboBoxSerif->setCurrentFont(f);
}

QString ContentTab::getSansSerifFont() const
{
    return ui->fontComboBoxSansSerif->currentFont().family();
}

void ContentTab::setSansSerifFont(const QString &fontFamily)
{
    QFont f(fontFamily);
    ui->fontComboBoxSansSerif->setFont(f);
    ui->fontComboBoxSansSerif->setCurrentFont(f);
}

QString ContentTab::getCursiveFont() const
{
    return ui->fontComboBoxCursive->currentFont().family();
}

void ContentTab::setCursiveFont(const QString &fontFamily)
{
    QFont f(fontFamily);
    ui->fontComboBoxCursive->setFont(f);
    ui->fontComboBoxCursive->setCurrentFont(f);
}

QString ContentTab::getFantasyFont() const
{
    return ui->fontComboBoxFantasy->currentFont().family();
}

void ContentTab::setFantasyFont(const QString &fontFamily)
{
    QFont f(fontFamily);
    ui->fontComboBoxFantasy->setFont(f);
    ui->fontComboBoxFantasy->setCurrentFont(f);
}

QString ContentTab::getFixedFont() const
{
    return ui->fontComboBoxFixed->currentFont().family();
}

void ContentTab::setFixedFont(const QString &fontFamily)
{
    QFont f(fontFamily);
    ui->fontComboBoxFixed->setFont(f);
    ui->fontComboBoxFixed->setCurrentFont(f);
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

int ContentTab::getFixedFontSize() const
{
    return ui->spinBoxFixedFontSize->value();
}

void ContentTab::setFixedFontSize(int size)
{
    if (size > 0)
        ui->spinBoxFixedFontSize->setValue(size);
}

bool ContentTab::isJavaScriptEnabled() const
{
    return ui->checkBoxEnableJavaScript->isChecked();
}

void ContentTab::toggleAdBlock(bool value)
{
    ui->checkBoxEnableAdBlock->setChecked(value);
}

void ContentTab::toggleAnimatedScrolling(bool value)
{
    ui->checkBoxEnableAnimatedScroll->setChecked(value);
}

void ContentTab::toggleAutoLoadImages(bool value)
{
    ui->checkBoxAutoLoadImages->setChecked(value);
}

void ContentTab::togglePlugins(bool value)
{
    ui->checkBoxEnablePlugins->setChecked(value);
}

void ContentTab::togglePopupBlock(bool value)
{
    ui->checkBoxBlockPopups->setChecked(value);
}

void ContentTab::toggleJavaScript(bool value)
{
    ui->checkBoxEnableJavaScript->setChecked(value);
}

void ContentTab::toggleUserScripts(bool value)
{
    ui->checkBoxEnableUserScripts->setChecked(value);
}
