#include "AutoFillDialog.h"
#include "ui_AutoFillDialog.h"

AutoFillDialog::AutoFillDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoFillDialog),
    m_actionType(AutoFillDialog::NewCredentialsAction),
    m_credentials()
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_X11NetWmWindowTypeCombo, true);
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setContentsMargins(0, 0, 0, 0);

    connect(ui->pushButtonSave,   &QPushButton::clicked, this, &AutoFillDialog::onSaveAction);
    connect(ui->pushButtonUpdate, &QPushButton::clicked, this, &AutoFillDialog::onUpdateAction);
    connect(ui->pushButtonRemove, &QPushButton::clicked, this, &AutoFillDialog::onRemoveAction);
    connect(ui->pushButtonIgnore, &QPushButton::clicked, this, &AutoFillDialog::reject);
}

AutoFillDialog::~AutoFillDialog()
{
    delete ui;
}

void AutoFillDialog::setAction(DialogAction action)
{
    m_actionType = action;

    switch (m_actionType)
    {
        case NewCredentialsAction:
            ui->pushButtonSave->show();
            ui->pushButtonUpdate->hide();
            ui->pushButtonRemove->hide();
            ui->labelMessage->setText(tr("Save login for %1?"));
            break;
        case UpdateCredentialsAction:
            ui->pushButtonSave->hide();
            ui->pushButtonUpdate->show();
            ui->pushButtonRemove->show();
            ui->labelMessage->setText(tr("Update login for %1?"));
            break;
    }
}

void AutoFillDialog::setInformation(const WebCredentials &credentials)
{
    m_credentials = credentials;

    ui->lineEditUsername->setText(m_credentials.Username);
    ui->lineEditPassword->setText(m_credentials.Password);
    ui->labelMessage->setText(ui->labelMessage->text().arg(credentials.Host));
}

void AutoFillDialog::alignAndShow(const QRect &windowGeom)
{
    QPoint dialogPos;
    dialogPos.setX(windowGeom.x() + windowGeom.width() / 11);
    dialogPos.setY(windowGeom.y() + 78);

    move(dialogPos);
    show();
    raise();
}

void AutoFillDialog::onSaveAction()
{
    updateWebCredentials();
    emit saveCredentials(m_credentials);

    close();
}

void AutoFillDialog::onUpdateAction()
{
    updateWebCredentials();
    emit updateCredentials(m_credentials);

    close();
}

void AutoFillDialog::onRemoveAction()
{
    emit removeCredentials(m_credentials);

    close();
}

void AutoFillDialog::updateWebCredentials()
{
    if (!ui->lineEditUsername->text().isEmpty())
        m_credentials.Username = ui->lineEditUsername->text();

    if (!ui->lineEditPassword->text().isEmpty())
        m_credentials.Password = ui->lineEditPassword->text();
}
