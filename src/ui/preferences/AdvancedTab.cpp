#include "AdvancedTab.h"
#include "ui_AdvancedTab.h"

#include <QString>

AdvancedTab::AdvancedTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AdvancedTab)
{
    ui->setupUi(this);

    // Populate the combobox for process model options 
    ui->comboBoxProcessModel->addItem(tr("Default"), QLatin1String(""));
    ui->comboBoxProcessModel->addItem(tr("Process per website"), QLatin1String("--process-per-site"));
    ui->comboBoxProcessModel->addItem(tr("Single process"), QLatin1String("--single-process"));

    connect(ui->checkBoxDisableGPU, &QCheckBox::clicked, this, &AdvancedTab::onGpuCheckBoxChanged);
}

AdvancedTab::~AdvancedTab()
{
    delete ui;
}

void AdvancedTab::setProcessModel(const QString &value)
{
    if (value.isEmpty())
    {
        ui->comboBoxProcessModel->setCurrentIndex(0);
        return;
    }

    for (int i = 0; i < ui->comboBoxProcessModel->count(); ++i)
    {
        if (ui->comboBoxProcessModel->itemData(i).toString().compare(value) == 0)
        {
            ui->comboBoxProcessModel->setCurrentIndex(i);
            return;
        }
    }
}

QString AdvancedTab::getProcessModel() const
{
    return ui->comboBoxProcessModel->currentData().toString();
}

void AdvancedTab::setGpuDisabled(bool value)
{
    // The UI shows the opposite of the internal setting.
    // ie when GPU is enabled, so is checkbox
    ui->checkBoxDisableGPU->setChecked(!value);
    onGpuCheckBoxChanged(!value);
}

bool AdvancedTab::isGpuDisabled() const
{
    return !ui->checkBoxDisableGPU->isChecked();
}

void AdvancedTab::onGpuCheckBoxChanged(bool value)
{
    ui->checkBoxDisableGPU->setText(value ? tr("Enabled") : tr("Disabled"));
}
