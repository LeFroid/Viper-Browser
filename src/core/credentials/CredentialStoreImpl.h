#ifndef CREDENTIALSTOREIMPL_H
#define CREDENTIALSTOREIMPL_H

#ifdef USE_KWALLET

#include "CredentialStoreKWallet.h"
using CredentialStoreImpl = CredentialStoreKWallet;

#else

#include "CredentialStore.h"

/// Empty class for unsupported systems
class CredentialStoreImpl final : public CredentialStore
{
public:
    CredentialStoreImpl() : CredentialStore() {}

    ~CredentialStoreImpl() {}

    std::vector<QString> getHostNames() override { return std::vector<QString>(); }

    void addCredentials(const WebCredentials &credentials) override {}

    std::vector<WebCredentials> getCredentialsFor(const QUrl &url) override { return std::vector<WebCredentials>(); }

    void removeCredentials(const WebCredentials &credentials) override {}

    void updateCredentials(const WebCredentials &credentials) override {}
};

#endif

#endif // CREDENTIALSTOREIMPL_H
