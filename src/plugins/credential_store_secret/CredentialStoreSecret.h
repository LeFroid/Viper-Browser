#ifndef CREDENTIALSTORESECRET_H
#define CREDENTIALSTORESECRET_H

#include "CredentialStore.h"

/**
 * @class CredentialStoreSecret
 * @brief Implementation of the credential store using libsecret as a backend storage system
 */
class CredentialStoreSecret : public QObject, public CredentialStore
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CredentialStore_iid FILE "credentialstoresecret.json")
    Q_INTERFACES(CredentialStore)

public:
    /// Constructor. Opens the libsecret backend
    explicit CredentialStoreSecret();

    /// Credential store destructor, frees resources used by libsecret
    ~CredentialStoreSecret();

    /// Returns a list of the hosts which have at least one set of credentials in the store
    std::vector<QString> getHostNames() override;

    /// Adds a set of credentials to the store
    void addCredentials(const WebCredentials &credentials) override;

    /// Returns a list of the credentials that have been saved for the given url
    std::vector<WebCredentials> getCredentialsFor(const QUrl &url) override;

    /// Removes the set of credentials from the store
    void removeCredentials(const WebCredentials &credentials) override;

    /// Updates the credentials
    void updateCredentials(const WebCredentials &credentials) override;

private:
    /// Loads credentials from the libsecret provider
    void loadCredentials();

    /// Saves the credentials associated with the given host into the libsecret provider
    void saveCredentialsFor(const QString &host);

private:
    /// Hashmap of host names to a container of the credentials associated with that host
    QHash<QString, std::vector<WebCredentials>> m_credentials;
};

#endif // CREDENTIALSTORESECRET_H
