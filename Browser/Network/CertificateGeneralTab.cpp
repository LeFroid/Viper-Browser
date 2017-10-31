#include "CertificateGeneralTab.h"
#include "ui_CertificateGeneralTab.h"

CertificateGeneralTab::CertificateGeneralTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CertificateGeneralTab)
{
    ui->setupUi(this);
}

CertificateGeneralTab::~CertificateGeneralTab()
{
    delete ui;
}

void CertificateGeneralTab::clearCertificate()
{
    QString emptyStr;
    ui->labelVerifiedForA->setText(emptyStr);
    ui->labelVerifiedForB->setText(emptyStr);
    ui->labelSubjectCN->setText(emptyStr);
    ui->labelSubjectOrg->setText(emptyStr);
    ui->labelSubjectOrgUnit->setText(emptyStr);
    ui->labelIssuerCN->setText(emptyStr);
    ui->labelIssuerOrg->setText(emptyStr);
    ui->labelIssuerOrgUnit->setText(emptyStr);
    ui->labelEffectiveDate->setText(emptyStr);
    ui->labelExpiryDate->setText(emptyStr);
    ui->labelSHA256->setText(emptyStr);
    ui->labelSHA1->setText(emptyStr);
}

void CertificateGeneralTab::setCertificate(const QSslCertificate &cert)
{
    QString notFoundText(tr("<Not Part Of Certificate>"));
    ui->labelVerifiedForA->setText(tr("SSL Server Certificate"));

    QStringList tmp = cert.subjectInfo(QSslCertificate::CommonName);
    ui->labelSubjectCN->setText(tmp.empty() ? notFoundText : tmp.at(0));
    tmp = cert.subjectInfo(QSslCertificate::Organization);
    ui->labelSubjectOrg->setText(tmp.empty() ? notFoundText : tmp.at(0));
    tmp = cert.subjectInfo(QSslCertificate::OrganizationalUnitName);
    ui->labelSubjectOrgUnit->setText(tmp.empty() ? notFoundText : tmp.at(0));

    tmp = cert.issuerInfo(QSslCertificate::CommonName);
    ui->labelIssuerCN->setText(tmp.empty() ? notFoundText : tmp.at(0));
    tmp = cert.issuerInfo(QSslCertificate::Organization);
    ui->labelIssuerOrg->setText(tmp.empty() ? notFoundText : tmp.at(0));
    tmp = cert.issuerInfo(QSslCertificate::OrganizationalUnitName);
    ui->labelIssuerOrgUnit->setText(tmp.empty() ? notFoundText : tmp.at(0));

    QDateTime certDate = cert.effectiveDate();
    ui->labelEffectiveDate->setText(certDate.toString("dddd, MMMM d, yyyy 'at' h:mm:ss AP"));

    certDate = cert.expiryDate();
    ui->labelExpiryDate->setText(certDate.toString("dddd, MMMM d, yyyy 'at' h:mm:ss AP"));

    ui->labelSHA256->setText(cert.digest(QCryptographicHash::Sha256).toHex());
    ui->labelSHA1->setText(cert.digest(QCryptographicHash::Sha1).toHex());
}
