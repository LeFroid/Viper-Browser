#ifndef SECURITYINFODIALOG_H
#define SECURITYINFODIALOG_H

#include "ServiceLocator.h"

#include <QList>
#include <QSslCertificate>
#include <QWidget>

namespace Ui {
    class SecurityInfoDialog;
}

class CertificateViewer;
class CookieJar;
class HistoryManager;
class QSslCertificate;

/**
 * @class SecurityInfoDialog
 * @brief Displays website information such as domain, owner, certificate (if HTTPS),
 *        how many times the user has visited the website, if the site uses cookies, etc.
 */
class SecurityInfoDialog : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the dialog given a pointer to the \ref CookieJar and \ref HistoryManager
    explicit SecurityInfoDialog(CookieJar *cookieJar, HistoryManager *historyManager);

    /// SecurityInfoDialog destructor
    ~SecurityInfoDialog();

    /// Sets the host name of the website, the security information of which will be
    /// displayed by the dialog. If a certificate chain is given, the user will be able
    /// to view this information in another dialog
    void setWebsite(const QUrl &url, const QString &host, const QList<QSslCertificate> &chain = QList<QSslCertificate>());

private:
    /// UI items contained in the SecurityInfoDialog.ui file
    Ui::SecurityInfoDialog *ui;

    /// Widget used to display detailed certificate information regarding the website being inspected
    CertificateViewer *m_certViewer;

    /// Main profile's cookie jar
    CookieJar *m_cookieJar;

    /// Main profile's history manager
    HistoryManager *m_historyManager;
};

#endif // SECURITYINFODIALOG_H
