#include "SecurityInfoDialog.h"
#include "ui_SecurityInfoDialog.h"

#include "BrowserApplication.h"
#include "CertificateViewer.h"
#include "CookieJar.h"
#include "HistoryManager.h"
#include <QSslCertificate>

SecurityInfoDialog::SecurityInfoDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SecurityInfoDialog),
    m_certViewer(new CertificateViewer)
{
    ui->setupUi(this);

    // Connect simple button signals to dialog display slots
    connect(ui->pushButtonCertificate, &QPushButton::clicked, m_certViewer, &CertificateViewer::show);
}

SecurityInfoDialog::~SecurityInfoDialog()
{
    delete m_certViewer;
    delete ui;
}

void SecurityInfoDialog::setWebsite(const QString &host, const QList<QSslCertificate> &chain)
{
    if (host.isEmpty() || host.isNull())
        return;

    ui->labelHostName->setText(host);

    // If no certificate chain is available, set the appropriate labels to reflect the lack of an HTTPS connection
    if (chain.empty())
    {
        m_certViewer->clearCertificateChain();
        ui->pushButtonCertificate->hide();
        ui->labelOwnerName->setText(tr("This website does not supply ownership information"));
        ui->labelVerifier->setText(tr("Not specified"));
        ui->labelIsEncrypted->setText(tr("Connection is not encrypted"));
        ui->labelIsEncryptedDetails->setText(tr("This website does not support encryption for the page you are viewing.\nInformation "
                                                "sent over the internet without encryption can be seen by others while it is in transit"));
    }
    else
    {
        m_certViewer->setCertificateChain(chain);
        ui->pushButtonCertificate->show();

        // Get some certificate info to display towards the top of the dialog
        const QSslCertificate &siteCert = chain.at(0);

        QStringList orgList = siteCert.subjectInfo(QSslCertificate::Organization);
        QString organization = orgList.empty() ? tr("This website does not supply ownership information") : orgList.at(0);
        ui->labelOwnerName->setText(organization);

        orgList = siteCert.issuerInfo(QSslCertificate::Organization);
        organization = orgList.empty() ? tr("Not specified") : orgList.at(0);
        ui->labelVerifier->setText(organization);

        ui->labelIsEncrypted->setText(tr("Connection is encrypted"));
        ui->labelIsEncryptedDetails->setText(tr("The page you are viewing was encrypted before transmission.\nEncryption keeps the "
                                                "information transmitted over a webpage secure and away from prying eyes."));
    }

    BrowserApplication *app = sBrowserApplication;
    int numVisits = app->getHistoryManager()->getTimesVisited(host);
    ui->labelTimesVisited->setText(numVisits > 0 ? QString("Yes, %1 times.").arg(numVisits) : QString("No"));

    // Check if website is storing cookies
    auto cookieJar = app->getCookieJar();
    if (cookieJar->hasCookiesFor(host))
    {
        ui->labelUsingCookies->setText(tr("Yes"));
        ui->pushButtonSiteCookies->show();
    }
    else
    {
        ui->labelUsingCookies->setText(tr("No"));
        ui->pushButtonSiteCookies->hide();
    }
}
