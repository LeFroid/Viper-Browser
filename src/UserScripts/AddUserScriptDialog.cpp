#include "AddUserScriptDialog.h"
#include "ui_AddUserScriptDialog.h"

AddUserScriptDialog::AddUserScriptDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddUserScriptDialog)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

    connect(this, &AddUserScriptDialog::accepted, this, &AddUserScriptDialog::onAccepted);
}

AddUserScriptDialog::~AddUserScriptDialog()
{
    delete ui;
}

void AddUserScriptDialog::onAccepted()
{
    emit informationEntered(ui->lineEditName->text(), ui->lineEditNamespace->text(),
                            ui->lineEditDescription->text(), ui->lineEditVersion->text());
}
