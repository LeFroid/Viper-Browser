#include "CookieJar.h"
#include "CookieWidget.h"
#include "HistoryManager.h"
#include "NetworkAccessManager.h"
#include "SecurityManager.h"
#include "SecurityInfoDialog.h"
#include "NetworkAccessManager.h"

#include <QApplication>
#include <QMessageBox>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSslCertificate>
#include <QSslError>
#include <QtGlobal>
#include <QDebug>

SecurityManager::SecurityManager(QObject *parent) :
    QObject(parent),
    m_cookieJar(nullptr),
    m_historyManager(nullptr),
    m_networkAccessManager(nullptr),
    m_insecureHosts(),
    m_exemptInsecureHosts(),
    m_certChains(),
    m_securityDialog(nullptr),
    m_cookieWidget(nullptr),
    m_needShowDialog(false),
    m_replyUrlTarget()
{
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

bool SecurityManager::onCertificateError(const QWebEngineCertificateError &certificateError, QWidget *window)
{
    auto hostName = certificateError.url().host();

    if (m_exemptInsecureHosts.contains(hostName))
        return true;

    m_insecureHosts.insert(hostName);

    QString warning = tr("The website you are attempting to load at \"%1\" is not secure.").arg(certificateError.url().host());
    QString preface = tr("The following errors were found:");
    QString option = tr("Do you wish to proceed?");

    QString messageBoxText = QString("%1<p>%2</p><ul><li>%3</li></ul><p>%4</p>").arg(warning, preface, certificateError.description(), option);
    QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No);
    int response = QMessageBox::question(window, tr("Security Threat"), messageBoxText, buttons, QMessageBox::NoButton);
    if (response == QMessageBox::Yes)
    {
        m_exemptInsecureHosts.insert(certificateError.url().host());
        return true;
    }

    return false;
}

void SecurityManager::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    m_cookieJar = serviceLocator.getServiceAs<CookieJar>("CookieJar");
    m_cookieWidget = serviceLocator.getServiceAs<CookieWidget>("CookieWidget");
    m_historyManager = serviceLocator.getServiceAs<HistoryManager>("HistoryManager");
    m_networkAccessManager = serviceLocator.getServiceAs<NetworkAccessManager>("NetworkAccessManager");

    connect(m_networkAccessManager, &NetworkAccessManager::finished,  this, &SecurityManager::onNetworkReply);
    connect(m_networkAccessManager, &NetworkAccessManager::sslErrors, this, &SecurityManager::onSSLErrors);
}

void SecurityManager::showSecurityInfo(const QUrl &url)
{
    if (url.isEmpty())
        return;

    if (!m_securityDialog)
        m_securityDialog = new SecurityInfoDialog(m_cookieJar, m_cookieWidget, m_historyManager);

    const bool isHttps = url.scheme().compare(QStringLiteral("https")) == 0;

    QString hostLower = url.host().toLower();
    auto certIt = m_certChains.find(hostLower);
    if (isHttps && certIt != m_certChains.end())
    {
        m_securityDialog->setWebsite(url, hostLower, certIt.value());
    }
    else
    {
        if (isHttps && m_networkAccessManager != nullptr)
        {
            m_needShowDialog = true;
            m_replyUrlTarget = url;

            QNetworkRequest request(url);
            QNetworkReply *reply = m_networkAccessManager->get(request);
            connect(reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
            connect(reply, &QNetworkReply::errorOccurred, [reply](QNetworkReply::NetworkError) {
                reply->deleteLater();
            });
#else
            connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), [reply](QNetworkReply::NetworkError){
                reply->deleteLater();
            });
#endif
            return;
        }

        m_securityDialog->setWebsite(url, hostLower);
    }

    m_securityDialog->show();
    m_securityDialog->raise();
    m_securityDialog->activateWindow();
}

void SecurityManager::onNetworkReply(QNetworkReply *reply)
{
    if (!reply || !reply->url().scheme().startsWith(QStringLiteral("https")))
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
    QString hostLower = host.toLower();
    if (m_certChains.find(hostLower) == m_certChains.end())
        m_certChains.insert(hostLower, certChain);

    for (const auto &errCode : sslErrors)
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

        m_securityDialog->setWebsite(reply->url(), hostLower, certChain);
        m_securityDialog->show();
        m_securityDialog->raise();
        m_securityDialog->activateWindow();
    }
}

void SecurityManager::onSSLErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    QString warning = tr("The website you are attempting to load at \"%1\" is not secure.").arg(reply->url().host());
    QString preface = tr("The following errors were found:");
    QString option = tr("Do you wish to proceed?");

    QString errorHtmlList = QStringLiteral("<ul>");
    for (const QSslError &err : qAsConst(errors))
        errorHtmlList.append(QString("<li>%1</li>").arg(err.errorString()));
    errorHtmlList.append(QStringLiteral("</ul>"));

    QString messageBoxText = QString("%1<p>%2</p>%3<p>%4</p>").arg(warning, preface, errorHtmlList, option);
    QMessageBox::StandardButtons buttons = QMessageBox::StandardButtons(QMessageBox::Yes | QMessageBox::No);
    int response = QMessageBox::question(QApplication::activeWindow(), tr("Security Threat"), messageBoxText, buttons, QMessageBox::NoButton);
    if (response == QMessageBox::Yes)
        reply->ignoreSslErrors(errors);

    m_insecureHosts.insert(reply->url().host());
}
