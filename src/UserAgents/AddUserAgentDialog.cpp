#include "AddUserAgentDialog.h"
#include "ui_AddUserAgentDialog.h"

#include <QInputDialog>

AddUserAgentDialog::AddUserAgentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddUserAgentDialog)
{
    ui->setupUi(this);

    connect(ui->pushButtonAddCategory, &QPushButton::clicked, this, &AddUserAgentDialog::showNewCategoryDialog);
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &AddUserAgentDialog::userAgentAdded);
}

AddUserAgentDialog::~AddUserAgentDialog()
{
    delete ui;
}

QString AddUserAgentDialog::getCategory() const
{
    return ui->comboBoxCategory->currentText();
}

QString AddUserAgentDialog::getAgentName() const
{
    return ui->lineEditAgentName->text();
}

QString AddUserAgentDialog::getAgentValue() const
{
    return ui->lineEditAgentValue->text();
}

void AddUserAgentDialog::hideNewCategoryOption()
{
    ui->labelAddCategory->hide();
    ui->pushButtonAddCategory->hide();
}

void AddUserAgentDialog::clearAgentCategories()
{
    ui->comboBoxCategory->clear();
}

void AddUserAgentDialog::addAgentCategory(const QString &name)
{
    ui->comboBoxCategory->addItem(name);
}

void AddUserAgentDialog::showNewCategoryDialog()
{
    bool ok;
    QString categoryName = QInputDialog::getText(this, tr("Add a Category"), tr("Name:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !categoryName.isEmpty())
    {
        ui->comboBoxCategory->addItem(categoryName);
        ui->comboBoxCategory->setCurrentIndex(ui->comboBoxCategory->count() - 1);
    }
}
