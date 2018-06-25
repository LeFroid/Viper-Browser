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
    m_exemptInsecureHosts(),
    m_certChains(),
    m_securityDialog(nullptr),
    m_needShowDialog(false),
    m_replyUrlTarget()
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

bool SecurityManager::onCertificateError(const QWebEngineCertificateError &certificateError)
{
    auto hostName = certificateError.url().host();

    if (m_exemptInsecureHosts.contains(hostName))
        return true;

    m_insecureHosts.insert(hostName);

    QString warning = tr("The website you are attempting to load is not secure.");
    QString preface = tr("The following errors were found:");
    QString option = tr("Do you wish to proceed?");

    QString messageBoxText = QString("%1<p>%2</p><ul><li>%3</li></ul><p>%4</p>").arg(warning, preface, certificateError.errorDescription(), option);
    QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No);
    int response = QMessageBox::question(nullptr, tr("Security Threat"), messageBoxText, buttons, QMessageBox::NoButton);
    if (response == QMessageBox::Yes)
    {
        m_exemptInsecureHosts.insert(certificateError.url().host());
        return true;
    }

    return false;
}

void SecurityManager::showSecurityInfo(const QUrl &url)
{
    if (url.isEmpty())
        return;

    if (!m_securityDialog)
        m_securityDialog = new SecurityInfoDialog;

    const bool isHttps = url.scheme().compare(QLatin1String("https")) == 0;

    QString hostStripped = url.host();
    hostStripped = hostStripped.remove(QRegExp("(www.)"));
    auto certIt = m_certChains.find(hostStripped);
    if (isHttps && certIt != m_certChains.end())
    {
        m_securityDialog->setWebsite(hostStripped, certIt.value());
    }
    else
    {
        if (isHttps)
        {
            m_needShowDialog = true;
            m_replyUrlTarget = url;

            QNetworkRequest request(url);
            QNetworkReply *reply = sBrowserApplication->getNetworkAccessManager()->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
            connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError){
                reply->deleteLater();
            });
            return;
        }

        m_securityDialog->setWebsite(hostStripped);
    }

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
        if (errCode.error() != QSslError::NoError && errCode.error() != QSslError::UnspecifiedError)
        {
            qDebug() << "Adding host " << host << " (url " << reply->url() << " , scheme " << reply->url().scheme() << ") to insecure host list, "
                        "for SSL Error: " << errCode.errorString();
            m_insecureHosts.insert(host);
            return;
        }
    }

    // remove from insecure list if this reply from same host contains no errors
    if (m_insecureHosts.contains(host))
        m_insecureHosts.remove(host);

    // Check if the security dialog needs to be shown in this callback
    if (m_needShowDialog && reply->url() == m_replyUrlTarget)
    {
        m_needShowDialog = false;
        m_replyUrlTarget = QUrl();

        m_securityDialog->setWebsite(hostStripped, certChain);
        m_securityDialog->show();
        m_securityDialog->raise();
        m_securityDialog->activateWindow();
    }
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
    m_insecureHosts.insert(reply->url().host());
}
