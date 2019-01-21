#ifndef CREDENTIALSTORE_H
#define CREDENTIALSTORE_H

#include <vector>

#include <QDataStream>
#include <QDateTime>
#include <QMap>
#include <QString>
#include <QUrl>
#include <QVariant>

/// Contains information used to log in to a certain website
struct WebCredentials
{
    /// Host location where the credentails are used
    QString Host;

    /// The last time that the credentials were used to log in to the website
    QDateTime LastLogin;

    /// Username or email value
    QString Username;

    /// Password value
    QString Password;

    /// Key-value pairs saved from the form.
    QMap<QString, QString> FormData;
};

QDataStream& operator<<(QDataStream &out, const WebCredentials &creds);
QDataStream& operator>>(QDataStream &in, WebCredentials &creds);

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

    /// Returns a list of the hosts which have at least one set of credentials in the store
    virtual std::vector<QString> getHostNames() = 0;

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
