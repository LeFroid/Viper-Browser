#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <vector>

#include <QMap>
#include <QString>
#include <QUrl>
#include <QVariant>

/// Contains information used to log in to a certain website
struct WebCredentials
{
    /// Host location where the credentails are used
    QString Host;

    /// Username or email value
    QString Username;

    /// Password value
    QString Password;

    /// Map of the key-value pairs from the form
    QMap<QString, QVariant> FormData;
};

/**
 * @class CredentialStore
 * @brief Interface used to access credentials encrypted on the
 *        user's system
 */
class CredentialStore
{
public:
    /// Default constructor
    CredentialStore();

    /// Credential store destructor
    virtual ~CredentialStore() {}

    /// Adds a set of credentials to the store
    virtual void addCredentials(const WebCredentials &credentials) = 0;

    /// Returns a list of the credentials that have been saved for the given url
    virtual std::vector<WebCredentials> getCredentialsFor(const QUrl &url) = 0;

    /// Removes the set of credentials from the store
    virtual void removeCredentials(const WebCredentials &credentials) = 0;

    /// Updates the credentials
    virtual void updateCredentials(const WebCredentials &credentials) = 0;
};

#endif // CREDENTIALSTORE_H
