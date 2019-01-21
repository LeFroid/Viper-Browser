#include "AuthDialog.h"
#include "ui_AuthDialog.h"

#include <QStyle>

AuthDialog::AuthDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AuthDialog)
{
    ui->setupUi(this);

    QIcon questionIcon(style()->standardIcon(QStyle::SP_MessageBoxQuestion));
    ui->labelIcon->setPixmap(questionIcon.pixmap(32, 32));

    ui->labelMessage->setWordWrap(true);
}

AuthDialog::~AuthDialog()
{
    delete ui;
}

void AuthDialog::setMessage(const QString &message)
{
    ui->labelMessage->setText(message);
}

QString AuthDialog::getUsername() const
{
    return ui->lineEditUsername->text();
}

QString AuthDialog::getPassword() const
{
    return ui->lineEditPassword->text();
}
