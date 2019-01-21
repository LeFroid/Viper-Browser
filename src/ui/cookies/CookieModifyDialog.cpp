#include "CookieModifyDialog.h"
#include "ui_CookieModifyDialog.h"

#include <QAbstractButton>
#include <QPushButton>

CookieModifyDialog::CookieModifyDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CookieModifyDialog),
    m_cookie()
{
    ui->setupUi(this);

    // Populate combo box texts
    ui->comboBoxSecure->addItem(tr("Any type of connection"), false);
    ui->comboBoxSecure->addItem(tr("Encrypted connections only"), true);

    ui->comboBoxHttpOnly->addItem(tr("Yes"), true);
    ui->comboBoxHttpOnly->addItem(tr("No"), false);

    ui->comboBoxExpireType->addItem(tr("Date"), static_cast<int>(ExpireType::Date));
    ui->comboBoxExpireType->addItem(tr("At the end of this session"), static_cast<int>(ExpireType::Session));

    // Set default expiration date to a year from now
    QDateTime nextYear = QDateTime::currentDateTime().addYears(1);
    ui->dateTimeEdit->setDateTime(nextYear);

    // Connect change in selection of expire combo box type to act of hiding or showing date time widget for expiration
    connect(ui->comboBoxExpireType, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CookieModifyDialog::onExpireTypeChanged);

    // Connect save button with cookie modification
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, &CookieModifyDialog::setCookieData);
}

CookieModifyDialog::~CookieModifyDialog()
{
    delete ui;
}

void CookieModifyDialog::setCookie(const QNetworkCookie &cookie)
{
    m_cookie = cookie;

    // Associate ui elements with new cookie
    ui->lineEditName->setText(m_cookie.name());
    ui->plainTextEditValue->document()->setPlainText(m_cookie.value());
    ui->lineEditDomain->setText(m_cookie.domain());
    ui->lineEditPath->setText(m_cookie.path());
    ui->comboBoxSecure->setCurrentIndex(m_cookie.isSecure() ? 1 : 0);
    ui->comboBoxHttpOnly->setCurrentIndex(m_cookie.isHttpOnly() ? 1 : 0);
    ui->comboBoxExpireType->setCurrentIndex(m_cookie.isSessionCookie() ? 1 : 0);
    if (!m_cookie.isSessionCookie())
        ui->dateTimeEdit->setDateTime(m_cookie.expirationDate());
}

const QNetworkCookie &CookieModifyDialog::getCookie() const
{
    return m_cookie;
}

void CookieModifyDialog::onExpireTypeChanged(int index)
{
    if (index < 0)
        return;
    ExpireType expType = static_cast<ExpireType>(ui->comboBoxExpireType->itemData(index).toInt());
    switch (expType)
    {
        case ExpireType::Date:
            ui->dateTimeEdit->show();
            break;
        case ExpireType::Session:
            ui->dateTimeEdit->hide();
            break;
    }
}

void CookieModifyDialog::setCookieData(QAbstractButton *button)
{
    if (button == ui->buttonBox->button(QDialogButtonBox::Save))
    {
        m_cookie.setName(ui->lineEditName->text().toUtf8());
        m_cookie.setValue(ui->plainTextEditValue->document()->toPlainText().toUtf8());
        m_cookie.setDomain(ui->lineEditDomain->text());
        m_cookie.setPath(ui->lineEditPath->text());
        m_cookie.setSecure(ui->comboBoxSecure->currentData().toBool());
        m_cookie.setHttpOnly(ui->comboBoxHttpOnly->currentData().toBool());

        ExpireType expType = static_cast<ExpireType>(ui->comboBoxExpireType->currentData().toInt());
        QDateTime expDateTime;
        if (expType == ExpireType::Date)
            expDateTime = ui->dateTimeEdit->dateTime();
        m_cookie.setExpirationDate(expDateTime);
        accept();
    }
    else
        reject();
}
