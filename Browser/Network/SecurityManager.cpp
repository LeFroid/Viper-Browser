#include "BrowserApplication.h"
#include "SecurityManager.h"
#include "SecurityInfoDialog.h"
#include "NetworkAccessManager.h"

#include <QMessageBox>
#include <QNetworkReply>
#include <QSslCertificate>
#include <QSslError>
#include <QDebug>

SecurityManager::SecurityManager(QObject *parent) :
    QObject(parent),
    m_insecureHosts(),
    m_certChains(),
    m_securityDialog(nullptr)
{
    BrowserApplication *app = sBrowserApplication;
    NetworkAccessManager *netAccessMgr = app->getNetworkAccessManager();
    NetworkAccessManager *privateNetAccessMgr = app->getPrivateNetworkAccessManager();

    connect(netAccessMgr, &NetworkAccessManager::finished, this, &SecurityManager::onNetworkReply);
    connect(netAccessMgr, &NetworkAccessManager::sslErrors, this, &SecurityManager::onSSLErrors);
    connect(privateNetAccessMgr, &NetworkAccessManager::finished, this, &SecurityManager::onNetworkReply);
    connect(privateNetAccessMgr, &NetworkAccessManager::sslErrors, this, &SecurityManager::onSSLErrors);
}

SecurityManager::~SecurityManager()
{
    if (m_securityDialog)
    {
        delete m_securityDialog;
        m_securityDialog = nullptr;
    }
}

SecurityManager &SecurityManager::instance()
{    
    static SecurityManager securityMgr;
    return securityMgr;
}

bool SecurityManager::isInsecure(const QString &host)
{
    return m_insecureHosts.contains(host);
}

void SecurityManager::showSecurityInfo(const QString &host)
{
    if (host.isEmpty())
        return;

    if (!m_securityDialog)
        m_securityDialog = new SecurityInfoDialog;

    auto certIt = m_certChains.find(host);
    if (certIt != m_certChains.end())
    {
        m_securityDialog->setWebsite(host, certIt.value());
    }
    else
        m_securityDialog->setWebsite(host);

    m_securityDialog->show();
    m_securityDialog->raise();
    m_securityDialog->activateWindow();
}

void SecurityManager::onNetworkReply(QNetworkReply *reply)
{
    if (!reply || !reply->url().scheme().startsWith("https"))
        return;

    QSslConfiguration sslConfig = reply->sslConfiguration();
    if (sslConfig.isNull())
        return;

    QString host = reply->url().host();

    // Skip if already found in insecure host list
    //if (m_insecureHosts.contains(host))
     //   return;

    QList<QSslCertificate> certChain = sslConfig.peerCertificateChain();
    QList<QSslError> sslErrors = QSslCertificate::verify(certChain, host);

    // Add host and their certificate chain to our hash map if not already there
    QString hostStripped = host.remove(QRegExp("(www.)"));
    if (m_certChains.find(hostStripped) == m_certChains.end())
        m_certChains.insert(hostStripped, certChain);

    for (auto errCode : sslErrors)
    {
        if (errCode.error() != QSslError::NoError)
        {
            qDebug() << "Adding host " << host << " (url " << reply->url() << " , scheme " << reply->url().scheme() << ") to insecure host list, "
                        "for SSL Error: " << errCode.errorString();
            m_insecureHosts.append(host);
            return;
        }
    }

    // remove from insecure list if this reply from same host contains no errors
    if (m_insecureHosts.contains(host))
        m_insecureHosts.removeAll(host);
}

void SecurityManager::onSSLErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QString message = tr("This website is not secure. The following errors were found: ");
    for (QSslError err : errors)
        message.append(QString("%1, ").arg(err.errorString()));
    message.chop(2);
    message.append(QString(". Do you wish to proceed?"));
    QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No);
    int response = QMessageBox::question(nullptr, tr("Security Threat"), message, buttons, QMessageBox::NoButton);
    if (response == QMessageBox::Yes)
        reply->ignoreSslErrors(errors);
    m_insecureHosts.append(reply->url().host());
}
