#ifndef CREDENTIALSTOREKWALLET_H
#define CREDENTIALSTOREKWALLET_H

#include "CredentialStore.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QtPlugin>

namespace KWallet {
    class Wallet;
}

/**
 * @class CredentialStoreKWallet
 * @brief Implementation of the credential store using KWallet as a backend storage system
 */
class CredentialStoreKWallet : public QObject, public CredentialStore
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID CredentialStore_iid FILE "credentialstorekwallet.json")
    Q_INTERFACES(CredentialStore)

public:
    /// Constructor. Opens the wallet backend
    explicit CredentialStoreKWallet();

    /// Credential store destructor, frees resources used by the wallet
    ~CredentialStoreKWallet();

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
    /// Attempts to open the network wallet and load any saved credentials into the store
    void openWallet();

    /// Saves the credentials associated with the given host into the wallet
    void saveCredentialsFor(const QString &host);

private:
    /// Pointer to the wallet used to store and retrieve credentials
    KWallet::Wallet *m_wallet;

    /// Hashmap of host names to a container of the credentials associated with that host
    QHash<QString, std::vector<WebCredentials>> m_credentials;
};

#endif // CREDENTIALSTOREKWALLET_H
