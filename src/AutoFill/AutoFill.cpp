#include "AutoFill.h"
#include "CredentialStoreImpl.h"
#include <type_traits>

AutoFill::AutoFill(QObject *parent) :
    QObject(parent),
    m_credentialStore(nullptr)
{
    if (std::is_class<CredentialStoreImpl>::value)
        m_credentialStore = std::make_unique<CredentialStoreImpl>();
}

AutoFill::~AutoFill()
{
}

void AutoFill::onFormSubmitted(const QString &/*pageUrl*/, const QString &/*username*/, const QString &/*password*/, const QByteArray &/*formData*/)
{

}

