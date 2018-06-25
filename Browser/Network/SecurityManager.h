#ifndef SECURITYMANAGER_H
#define SECURITYMANAGER_H

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QUrl>
#include <QWebEngineCertificateError>

class SecurityInfoDialog;
class QNetworkReply;
class QSslCertificate;
class QSslError;

/**
 * @class SecurityManager
 * @brief Scans network activity for encrypted connections, verifying their
 *        authenticity and storing relevant security information (such as
 *        certificate chains) so the user may examine them.
 */
class SecurityManager : public QObject
{
    Q_OBJECT
public:
    explicit SecurityManager(QObject *parent = nullptr);
    ~SecurityManager();

    /// Returns the security manager singleton
    static SecurityManager &instance();

    /// Returns true if the given host is known to have an invalid certificate, false if else
    bool isInsecure(const QString &host);

    /// Called by a \ref WebPage when the given certificate error has been encountered during a page load
    bool onCertificateError(const QWebEngineCertificateError &certificateError);

public slots:
    /// Shows a dialog containing the certificate information (or lack thereof) for the host of the given url
    void showSecurityInfo(const QUrl &url);

private slots:
    /// Called when any network reply has been received - checks if reply is associated with current tab,
    /// if connection is over SSL the certificate's validity will be confirmed
    void onNetworkReply(QNetworkReply *reply);

    /// Called when an SSL connection encounters some errors during set up
    void onSSLErrors(QNetworkReply *reply, const QList<QSslError> &errors);

private:
    /// List of known insecure hosts with invalid certificates
    QSet<QString> m_insecureHosts;

    /// Set of insecure hosts with user-specified exemptions allowing them to be loaded
    QSet<QString> m_exemptInsecureHosts;

    /// Hash map of hosts using HTTPS protocol and their corresponding certificate chains
    QHash< QString, QList<QSslCertificate> > m_certChains;

    /// Security information dialog for websites
    SecurityInfoDialog *m_securityDialog;

    /// True if the SecurityInfoDialog needs to be shown after fetching a website's SSL certificate chain, false if else
    bool m_needShowDialog;

    /// The URL being searched for in the onNetworkReply slot if m_needShowDialog is true.
    QUrl m_replyUrlTarget;
};

#endif // SECURITYMANAGER_H
