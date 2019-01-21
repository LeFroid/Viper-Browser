#ifndef CERTIFICATEVIEWER_H
#define CERTIFICATEVIEWER_H

#include <QList>
#include <QSslCertificate>
#include <QWidget>

namespace Ui {
    class CertificateViewer;
}

class QFormLayout;
class QLabel;
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;

/// Certificate fields displayed in the Details tree widget
enum class CertificateField
{
    //todo: add values for certificate extensions
    None = 0,
    Version,
    SerialNumber,
    Issuer,
    EffectiveDate,
    ExpiryDate,
    Subject,
    SubjectPubKeyAlgorithm,
    SubjectPubKey,
    SignatureAlgorithm,
    Signature,
    SHA256Fingerprint,
    SHA1Fingerprint
};

/// Convenience template used in this case to cast an enum into its underlying type (\ref CertificateField)
template <typename E> constexpr auto to_underlying(E e) noexcept { return static_cast<std::underlying_type_t<E>>(e); }

/// Casts the given integer into a \ref CertificateField value
constexpr CertificateField to_certfield(int value) { return static_cast<CertificateField>(value); }

/**
 * @class CertificateViewer
 * @brief Displays information regarding a domain's SSL certificate and the
 *        chain it belongs to, up to and including the trusted root certificate
 *        authority
 */
class CertificateViewer : public QWidget
{
    Q_OBJECT

public:
    explicit CertificateViewer(QWidget *parent = 0);
    ~CertificateViewer();

    /// Clears any existing certificate chain from use, resetting the UI elements associated with that chain
    void clearCertificateChain();

    /// Sets the certificate chain to be used by the widget
    void setCertificateChain(const QList<QSslCertificate> &chain);

private slots:
    /// Called when an item in the certificate hierarchy tree view has been selected
    void onCertChainItemSelected(QTreeWidgetItem *item, int column);

    /// Called when a field has been selected to be viewed in the field view widget
    void onCertFieldSelected();

private:
    /// Initializes the items found in the detailed tab
    void setupDetailTab();

    /// Creates and returns a \ref QFormLayout for the general tab, setting some default properties used by all
    /// form layouts of that tab
    QFormLayout *makeFormLayout() const;

    /**
     * @brief makeCertField Creates a tree widget item in the certificate fields \ref QTreeWidget
     * @param parent Parent item of the widget
     * @param text Text to be displayed for the tree item
     * @param field Certificate field corresponding to the item (defaults to none)
     * @return A pointer to the newly created QTreeWidgetItem
     */
    QTreeWidgetItem *makeCertField(QTreeWidgetItem *parent, const QString &text, CertificateField field = CertificateField::None);

private:
    /// Entities for which any type of QSslCertificate::SubjectInfo may be obtained
    enum class CertInfoType { Subject, Issuer };

    /**
     * @brief Fetches the requested entity information, returning the result in string form
     * @param cert The SSL certificate
     * @param info The type of information being requested
     * @param type The type of entity for which the information is being requested
     * @return A string containing the subject information
     */
    inline QString getSubjectInfoString(const QSslCertificate &cert, QSslCertificate::SubjectInfo info, CertInfoType type) const
    {
        QStringList infoList;
        switch (type)
        {
            case CertInfoType::Subject:
                infoList = cert.subjectInfo(info);
                break;
            case CertInfoType::Issuer:
                infoList = cert.issuerInfo(info);
                break;
        }
        if (infoList.empty())
            return QString();

        QString infoStr;
        switch (info)
        {
            case QSslCertificate::Organization:
                infoStr = QStringLiteral("O = ");
                break;
            case QSslCertificate::CommonName:
                infoStr = QStringLiteral("CN = ");
                break;
            case QSslCertificate::LocalityName:
                infoStr = QStringLiteral("L = ");
                break;
            case QSslCertificate::OrganizationalUnitName:
                infoStr = QStringLiteral("OU = ");
                break;
            case QSslCertificate::CountryName:
                infoStr = QStringLiteral("C = ");
                break;
            case QSslCertificate::StateOrProvinceName:
                infoStr = QStringLiteral("ST = ");
                break;
            default:
                break;
        }
        infoStr.append(infoList.at(0)).append(QChar('\n'));
        return infoStr;
    }

private:
    /// UI elements found in CertificateViewer.ui file
    Ui::CertificateViewer *ui;

    /// Certificate chain of the website being viewed, if applicable
    QList<QSslCertificate> m_certChain;

    /// Index of the certificate being viewed in the Details tab
    int m_currentCertIdx;

    // Items belonging to the Details tab:
    QLabel *labelCertHierarchyStatic, *labelCertFieldsStatic, *labelFieldValueStatic;

    /// Widgets used to displays the certificate hierarchy and the fields of the selected certificate
    QTreeWidget *treeCertHierarchy, *treeCertFields;

    /// Displays the value of the selected certificate field
    QTextEdit *textEditFieldValue;
};

#endif // CERTIFICATEVIEWER_H
