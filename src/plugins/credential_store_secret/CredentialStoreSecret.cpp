#include <libsecret/secret.h>
#include "./CredentialStoreSecret.h"
#include <QUrlQuery>
#include <QDebug>

static bool compareWebCredentials(const WebCredentials &a, const WebCredentials &b)
{
    return a.LastLogin < b.LastLogin;
}

CredentialStoreSecret::CredentialStoreSecret() :
    QObject(),
    m_credentials()
{
    setObjectName(QStringLiteral("CredentialStoreSecret"));
    loadCredentials();
}

CredentialStoreSecret::~CredentialStoreSecret()
{
}

std::vector<QString> CredentialStoreSecret::getHostNames()
{
    std::vector<QString> result;

    for (auto it = m_credentials.cbegin(); it != m_credentials.cend(); ++it)
        result.push_back(it.key());

    return result;
}

void CredentialStoreSecret::addCredentials(const WebCredentials &credentials)
{
    if (credentials.Host.isEmpty())
        return;

    std::vector<WebCredentials> &creds = m_credentials[credentials.Host];
    creds.push_back(credentials);

    std::sort(creds.begin(), creds.end(), compareWebCredentials);

    saveCredentialsFor(credentials.Host);
}

std::vector<WebCredentials> CredentialStoreSecret::getCredentialsFor(const QUrl &url)
{
    const QString host = url.host();
    if (!m_credentials.contains(host))
        return std::vector<WebCredentials>();

    return m_credentials.value(host);
}

void CredentialStoreSecret::removeCredentials(const WebCredentials &credentials)
{
    if (!m_credentials.contains(credentials.Host))
        return;

    std::vector<WebCredentials> &creds = m_credentials[credentials.Host];
    for (auto it = creds.begin(); it != creds.end();)
    {
        if (it->Username.compare(credentials.Username) == 0)
            it = creds.erase(it);
        else
            ++it;
    }

    std::sort(creds.begin(), creds.end(), compareWebCredentials);

    saveCredentialsFor(credentials.Host);
}

void CredentialStoreSecret::updateCredentials(const WebCredentials &credentials)
{
    if (!m_credentials.contains(credentials.Host))
        return;

    std::vector<WebCredentials> &creds = m_credentials[credentials.Host];

    bool updated = false;
    for (WebCredentials &item : creds)
    {
        if (item.Username.compare(credentials.Username) == 0)
        {
            item.Password = credentials.Password;
            item.FormData = credentials.FormData;
            item.LastLogin = credentials.LastLogin;
            updated = true;
        }
    }

    if (updated)
    {
        std::sort(creds.begin(), creds.end(), compareWebCredentials);
        saveCredentialsFor(credentials.Host);
    }
    else
        addCredentials(credentials);
}

static const SecretSchema schema =
{
    "org.viper-browser.web-credential",
    SECRET_SCHEMA_NONE,
    {
        {      "host", SECRET_SCHEMA_ATTRIBUTE_STRING },
        { "lastlogin", SECRET_SCHEMA_ATTRIBUTE_STRING },
        {  "username", SECRET_SCHEMA_ATTRIBUTE_STRING },
        {  "formdata", SECRET_SCHEMA_ATTRIBUTE_STRING },
        {                                             }
    }
};

void CredentialStoreSecret::saveCredentialsFor(const QString &host)
{
    const std::vector<WebCredentials> &creds = m_credentials[host];
    GError *error = NULL;

    secret_password_clear_sync(&schema, NULL, &error, "host", host.toStdString().c_str(), NULL);

    if (error != NULL)
    {
        qDebug() << "CredentialStoreSecret: could not clear credentials: " << error->message;
        g_error_free(error);
        return;
    }

    for (const WebCredentials &item : creds)
    {
        const std::string host = item.Host.toStdString();
        const std::string lastlogin = item.LastLogin.toString(Qt::ISODate).toStdString();
        const std::string username = item.Username.toStdString();
        const std::string password = item.Password.toStdString();
        std::string formdata;
        const std::string label = (item.Host + ':' + item.Username).toStdString();
        QUrlQuery querystring;
        auto &FormData = item.FormData;

        for (auto it = FormData.begin(); it != FormData.end(); ++it)
        {
            querystring.addQueryItem(it.key(), it.value());
        }

        formdata = querystring.toString(QUrl::FullyEncoded).toStdString();

        secret_password_store_sync(
            &schema,
            SECRET_COLLECTION_DEFAULT,
            label.c_str(),
            password.c_str(),
            NULL,
            &error,
            "host",
            host.c_str(),
            "lastlogin",
            lastlogin.c_str(),
            "username",
            username.c_str(),
            "formdata",
            formdata.c_str(),
            NULL
        );

        if (error != NULL)
        {
            qDebug() << "CredentialStoreSecret: could not store credentials: " << error->message;
            g_error_free(error);
            return;
        }
    }
}

void CredentialStoreSecret::loadCredentials()
{
    const SecretSearchFlags flags = SECRET_SEARCH_ALL;
    GError *error = NULL;
    GList *credentials = secret_password_search_sync(&schema, flags, NULL, &error, NULL);

    if (credentials == NULL)
    {
        if (error != NULL)
        {
            qDebug() << "CredentialStoreSecret: could not load credentials: " << error->message;
            g_error_free(error);
        }
        return;
    }

    for (; credentials != NULL; credentials = credentials->next)
    {
        SecretRetrievable *credential = static_cast<SecretRetrievable *>(credentials->data);
        SecretValue *secret = secret_retrievable_retrieve_secret_sync(credential, NULL, &error);

        if (secret == NULL)
        {
            if (error == NULL)
            {
                qDebug() << "CredentialStoreSecret: could not load password";
            }
            else
            {
                qDebug() << "CredentialStoreSecret: could not load password: " << error->message;
                g_error_free(error);
                error = NULL;
            }
            continue;
        }

        GHashTable *attributes = secret_retrievable_get_attributes(credential);
        const gchar *password = secret_value_get_text(secret);
        const gchar *host = static_cast<const gchar *>(g_hash_table_lookup(attributes, "host"));
        const gchar *lastlogin = static_cast<const gchar *>(g_hash_table_lookup(attributes, "lastlogin"));
        const gchar *username = static_cast<const gchar *>(g_hash_table_lookup(attributes, "username"));
        const gchar *formdata = static_cast<const gchar *>(g_hash_table_lookup(attributes, "formdata"));

        if (password == NULL || host == NULL || lastlogin == NULL || username == NULL || formdata == NULL)
        {
            qDebug() << "CredentialStoreSecret: could not load attributes";
        }
        else
        {
            WebCredentials item = {host, QDateTime::fromString(lastlogin, Qt::ISODate), username, password, {}};
            QUrlQuery querystring(formdata);
            auto pairs = querystring.queryItems(QUrl::FullyDecoded);

            for (auto it = pairs.begin(); it != pairs.end(); ++it)
            {
                item.FormData[it->first] = it->second;
            }

            m_credentials[host].push_back(item);
        }

        secret_value_unref(secret);
        g_hash_table_unref(attributes);
    }

    for (std::vector<WebCredentials> &creds : m_credentials)
    {
        std::sort(creds.begin(), creds.end(), compareWebCredentials);
    }
}
