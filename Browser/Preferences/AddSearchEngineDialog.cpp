#include "AddSearchEngineDialog.h"
#include "ui_AddSearchEngineDialog.h"

AddSearchEngineDialog::AddSearchEngineDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddSearchEngineDialog)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, [=](){
        QString engineName = ui->lineEditName->text(), queryUrl = ui->lineEditQueryURL->text();
        if (!engineName.isEmpty() && !queryUrl.isEmpty())
            emit searchEngineAdded(engineName, queryUrl);
    });
}

void AddSearchEngineDialog::clearValues()
{
    ui->lineEditName->clear();
    ui->lineEditQueryURL->clear();
}

AddSearchEngineDialog::~AddSearchEngineDialog()
{
    delete ui;
}
