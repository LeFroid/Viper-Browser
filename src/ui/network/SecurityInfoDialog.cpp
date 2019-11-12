#include "SecurityInfoDialog.h"
#include "ui_SecurityInfoDialog.h"

#include "CertificateViewer.h"
#include "CookieJar.h"
#include "CookieWidget.h"
#include "HistoryManager.h"

#include <QSslCertificate>

SecurityInfoDialog::SecurityInfoDialog(CookieJar *cookieJar, CookieWidget *cookieWidget, HistoryManager *historyManager) :
    QWidget(nullptr),
    ui(new Ui::SecurityInfoDialog),
    m_hostName(),
    m_certViewer(new CertificateViewer),
    m_cookieJar(cookieJar),
    m_cookieWidget(cookieWidget),
    m_historyManager(historyManager)
{
    ui->setupUi(this);

    // Connect simple button signals to dialog display slots
    connect(ui->pushButtonCertificate, &QPushButton::clicked, m_certViewer, &CertificateViewer::show);
    connect(ui->pushButtonSiteCookies, &QPushButton::clicked, this, &SecurityInfoDialog::showCookieWidget);
}

SecurityInfoDialog::~SecurityInfoDialog()
{
    delete m_certViewer;
    delete ui;
}

void SecurityInfoDialog::setWebsite(const QUrl &url, const QString &host, const QList<QSslCertificate> &chain)
{
    if (host.isEmpty() || host.isNull())
        return;

    m_hostName = host;
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

    m_historyManager->getTimesVisitedHost(url, [this](int numVisits){
        ui->labelTimesVisited->setText(numVisits > 0 ? QString("Yes, %1 times.").arg(numVisits) : QString("No"));
    });

    // Check if website is storing cookies
    if (m_cookieJar->hasCookiesFor(host))
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

void SecurityInfoDialog::showCookieWidget()
{
    m_cookieWidget->resetUI();
    m_cookieWidget->searchForHost(m_hostName);
    m_cookieWidget->show();
    m_cookieWidget->raise();
    m_cookieWidget->activateWindow();
}
