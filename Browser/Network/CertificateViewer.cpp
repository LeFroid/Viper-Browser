#include "CertificateViewer.h"
#include "ui_CertificateViewer.h"
#include "CertificateGeneralTab.h"

#include <QDateTime>
#include <QFormLayout>
#include <QLabel>
#include <QSpacerItem>
#include <QSslKey>
#include <QTextEdit>
#include <QTreeWidget>

CertificateViewer::CertificateViewer(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CertificateViewer),
    m_certChain(),
    m_currentCertIdx(-1)
{
    ui->setupUi(this);

    setupDetailTab();
}

CertificateViewer::~CertificateViewer()
{
    delete ui;
}

void CertificateViewer::clearCertificateChain()
{
    m_certChain.clear();
}

void CertificateViewer::setCertificateChain(const QList<QSslCertificate> &chain)
{
    if (chain.empty())
        return;

    m_certChain = chain;

    // Attempt to find values and populate certificate info labels in general tab
    QString notFoundText(tr("<Not Part Of Certificate>"));
    const QSslCertificate &cert = m_certChain.at(0);
    if (cert.isNull())
    {
        // Clear all dynamically populated UI elements
        ui->tabGeneral->clearCertificate();
        return;
    }

    ui->tabGeneral->setCertificate(cert);

    // Populate details tab
    treeCertHierarchy->clear();
    textEditFieldValue->setPlainText(QString());

    // Certificate hierarchy tree widget items
    int certChainSize = m_certChain.size();

    QTreeWidgetItem *hierarchyItem = new QTreeWidgetItem(treeCertHierarchy);
    QStringList cnList = m_certChain.at(certChainSize - 1).subjectInfo(QSslCertificate::CommonName);
    hierarchyItem->setText(0, (cnList.empty() ? "Unknown Name" : cnList.at(0)));
    hierarchyItem->setData(0, Qt::UserRole, certChainSize - 1);

    int startIdx = (certChainSize > 1) ? certChainSize - 2 : -1;
    for (int i = startIdx; i >= 0; --i)
    {
        const QSslCertificate &treeCertItem = m_certChain.at(i);
        QTreeWidgetItem *certItem = new QTreeWidgetItem(hierarchyItem);

        cnList = treeCertItem.subjectInfo(QSslCertificate::CommonName);
        certItem->setText(0, (cnList.empty() ? "Unknown Name" : cnList.at(0)));
        certItem->setData(0, Qt::UserRole, i);

        hierarchyItem = certItem;
    }
    treeCertHierarchy->expandAll();

    QTreeWidgetItem *certFieldCN = treeCertFields->topLevelItem(0);
    if (certFieldCN)
    {
        certFieldCN->setText(0, hierarchyItem->text(0));
        m_currentCertIdx = 0;
    }
}

void CertificateViewer::onCertChainItemSelected(QTreeWidgetItem *item, int /*column*/)
{
    if (!item)
        return;

    int newCertIdx = item->data(0, Qt::UserRole).toInt();
    if (newCertIdx < 0 || newCertIdx >= m_certChain.size() || newCertIdx == m_currentCertIdx)
        return;

    m_currentCertIdx = newCertIdx;

    QTreeWidgetItem *certFieldCN = treeCertFields->topLevelItem(0);
    if (certFieldCN)
    {
        const QSslCertificate &cert = m_certChain.at(m_currentCertIdx);
        QStringList certCNList = cert.subjectInfo(QSslCertificate::CommonName);
        certFieldCN->setText(0, certCNList.empty() ? "Unknown CN" : certCNList.at(0));
    }

    // Check if a value from the last selected item is being displayed in the field value widget,
    // if so, then update it to the value associated with the new certificate
    if (!textEditFieldValue->toPlainText().isEmpty())
    {
        if (!treeCertFields->selectedItems().empty())
            onCertFieldSelected();
    }
}

void CertificateViewer::onCertFieldSelected()
{
    auto selection = treeCertFields->selectedItems();
    if (selection.size() != 1)
        return;

    const QTreeWidgetItem *item = selection.at(0);
    if (!item || m_currentCertIdx < 0)
        return;

    bool ok;
    int userData = item->data(0, Qt::UserRole).toInt(&ok);
    if (!ok)
        return;

    const QSslCertificate &cert = m_certChain.at(m_currentCertIdx);

    QString contents;
    switch (to_certfield(userData))
    {
        case CertificateField::Version:
            contents = cert.version();
            break;
        case CertificateField::SerialNumber:
            contents = cert.serialNumber();
            break;
        case CertificateField::Issuer:
        {
            contents = getSubjectInfoString(cert, QSslCertificate::CommonName, CertInfoType::Issuer);
            contents.append(getSubjectInfoString(cert, QSslCertificate::Organization, CertInfoType::Issuer));
            contents.append(getSubjectInfoString(cert, QSslCertificate::OrganizationalUnitName, CertInfoType::Issuer));
            contents.append(getSubjectInfoString(cert, QSslCertificate::LocalityName, CertInfoType::Issuer));
            contents.append(getSubjectInfoString(cert, QSslCertificate::StateOrProvinceName, CertInfoType::Issuer));
            contents.append(getSubjectInfoString(cert, QSslCertificate::CountryName, CertInfoType::Issuer));
            break;
        }
        case CertificateField::EffectiveDate:
            contents = cert.effectiveDate().toString("M/d/yy, h:mm:ss AP t");
            break;
        case CertificateField::ExpiryDate:
            contents = cert.expiryDate().toString("M/d/yy, h:mm:ss AP t");
            break;
        case CertificateField::Subject:
        {
            contents = getSubjectInfoString(cert, QSslCertificate::CommonName, CertInfoType::Subject);
            contents.append(getSubjectInfoString(cert, QSslCertificate::Organization, CertInfoType::Subject));
            contents.append(getSubjectInfoString(cert, QSslCertificate::OrganizationalUnitName, CertInfoType::Subject));
            contents.append(getSubjectInfoString(cert, QSslCertificate::LocalityName, CertInfoType::Subject));
            contents.append(getSubjectInfoString(cert, QSslCertificate::StateOrProvinceName, CertInfoType::Subject));
            contents.append(getSubjectInfoString(cert, QSslCertificate::CountryName, CertInfoType::Subject));
            break;
        }
        case CertificateField::SubjectPubKeyAlgorithm:
        {
            switch (cert.publicKey().algorithm())
            {
                case QSsl::Rsa:
                    contents = QStringLiteral("RSA Algorithm");
                    break;
                case QSsl::Dsa:
                    contents = QStringLiteral("DSA Algorithm");
                    break;
                case QSsl::Ec:
                    contents = QStringLiteral("Elliptic Curve Algorithm");
                    break;
                case QSsl::Opaque:
                    contents = QStringLiteral("Opaque");
                    break;
            }

            break;
        }
        case CertificateField::SubjectPubKey:
            contents = cert.publicKey().toDer().toHex();
            break;
        case CertificateField::SignatureAlgorithm:
        {
            QStringList findAlgStr = cert.toText().split("Signature Algorithm: ");
            if (findAlgStr.size() > 1)
            {
                findAlgStr = findAlgStr.at(1).split('\n');
                if (!findAlgStr.empty())
                    contents = findAlgStr.at(0);
            }
            break;
        }
        case CertificateField::Signature:
        {
            QStringList findAlgStr = cert.toText().split("Signature Algorithm: ");
            if (findAlgStr.size() > 2)
            {
                findAlgStr = findAlgStr.at(2).split('\n');
                if (findAlgStr.size() > 1)
                {
                    for (int i = 1; i < findAlgStr.size(); ++i)
                        contents.append(findAlgStr.at(i)).append('\n');
                }
            }
            break;
        }
        case CertificateField::SHA256Fingerprint:
            contents = cert.digest(QCryptographicHash::Sha256).toHex();
            break;
        case CertificateField::SHA1Fingerprint:
            contents = cert.digest(QCryptographicHash::Sha1).toHex();
            break;
        case CertificateField::None:
        default:
            break;
    }
    textEditFieldValue->setPlainText(contents);
}

void CertificateViewer::setupDetailTab()
{
    QVBoxLayout *tabLayout = new QVBoxLayout;
    ui->tabDetails->setLayout(tabLayout);

    labelCertHierarchyStatic = new QLabel(tr("Certificate Hierarchy"), ui->tabDetails);
    tabLayout->addWidget(labelCertHierarchyStatic);

    treeCertHierarchy = new QTreeWidget(ui->tabDetails);
    treeCertHierarchy->setHeaderHidden(true);
    treeCertHierarchy->setColumnCount(1);
    tabLayout->addWidget(treeCertHierarchy);

    labelCertFieldsStatic = new QLabel(tr("Certificate Fields"), ui->tabDetails);
    tabLayout->addWidget(labelCertFieldsStatic);

    treeCertFields = new QTreeWidget(ui->tabDetails);
    treeCertFields->setHeaderHidden(true);
    treeCertFields->setColumnCount(1);
    tabLayout->addWidget(treeCertFields);

    labelFieldValueStatic = new QLabel(tr("Field Value"), ui->tabDetails);
    tabLayout->addWidget(labelFieldValueStatic);

    textEditFieldValue = new QTextEdit(ui->tabDetails);
    textEditFieldValue->setReadOnly(true);
    tabLayout->addWidget(textEditFieldValue);

    // Populate certificate fields widget (static field names except root item text)
    QTreeWidgetItem *certFieldRoot = new QTreeWidgetItem(treeCertFields);
    certFieldRoot->setText(0, tr("Placeholder: Common Name"));

    QTreeWidgetItem *certInfo = new QTreeWidgetItem(certFieldRoot);
    certInfo->setText(0, tr("Certificate"));

    static_cast<void>(makeCertField(certInfo, tr("Version"), CertificateField::Version));
    static_cast<void>(makeCertField(certInfo, tr("Serial Number"), CertificateField::SerialNumber));
    static_cast<void>(makeCertField(certInfo, tr("Issuer"), CertificateField::Issuer));

    QTreeWidgetItem *certInfoItem = makeCertField(certInfo, tr("Validity"));
    static_cast<void>(makeCertField(certInfoItem, tr("Not Before"), CertificateField::EffectiveDate));
    static_cast<void>(makeCertField(certInfoItem, tr("Not After"), CertificateField::ExpiryDate));

    static_cast<void>(makeCertField(certInfo, tr("Subject"), CertificateField::Subject));

    certInfoItem = makeCertField(certInfo, tr("Subject Public Key Info"));
    static_cast<void>(makeCertField(certInfoItem, tr("Subject Public Key Algorithm"), CertificateField::SubjectPubKeyAlgorithm));
    static_cast<void>(makeCertField(certInfoItem, tr("Subject Public Key"), CertificateField::SubjectPubKey));

    certInfoItem = makeCertField(certInfo, tr("Extensions"));
    //TODO: sub-items for extensions

    static_cast<void>(makeCertField(certInfo, tr("Certificate Signature Algorithm"), CertificateField::SignatureAlgorithm));
    static_cast<void>(makeCertField(certInfo, tr("Certificate Signature Value"), CertificateField::Signature));

    certInfoItem = makeCertField(certInfo, tr("Fingerprints"));
    static_cast<void>(makeCertField(certInfoItem, tr("SHA-256 Fingerprint"), CertificateField::SHA256Fingerprint));
    static_cast<void>(makeCertField(certInfoItem, tr("SHA-1 Fingerprint"), CertificateField::SHA1Fingerprint));

    treeCertFields->expandToDepth(2);

    connect(treeCertHierarchy, &QTreeWidget::itemClicked, this, &CertificateViewer::onCertChainItemSelected);
    connect(treeCertFields, &QTreeWidget::itemSelectionChanged, this, &CertificateViewer::onCertFieldSelected);
}

QFormLayout *CertificateViewer::makeFormLayout() const
{
    QMargins formMargins(50, 0, 0, 0);
    Qt::Alignment formLabelAlign = Qt::AlignLeft | Qt::AlignTop;

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(formLabelAlign);
    formLayout->setContentsMargins(formMargins);
    return formLayout;
}

QTreeWidgetItem *CertificateViewer::makeCertField(QTreeWidgetItem *parent, const QString &text, CertificateField field)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(parent);
    item->setText(0, text);
    item->setData(0, Qt::UserRole, to_underlying(field));
    return item;
}
