#ifndef CERTIFICATEGENERALTAB_H
#define CERTIFICATEGENERALTAB_H

#include <QSslCertificate>
#include <QWidget>

namespace Ui {
    class CertificateGeneralTab;
}

/**
 * @class CertificateGeneralTab
 * @brief Contains website-specific certificate information, without displaying details such as the
 *        public key algorithm and such
 */
class CertificateGeneralTab : public QWidget
{
    Q_OBJECT

public:
    explicit CertificateGeneralTab(QWidget *parent = 0);
    ~CertificateGeneralTab();

    /// Clears any existing certificate from use, resetting the UI elements associated with it
    void clearCertificate();

    /// Sets the certificate to be displayed by the widget
    void setCertificate(const QSslCertificate &cert);

private:
    Ui::CertificateGeneralTab *ui;
};

#endif // CERTIFICATEGENERALTAB_H
