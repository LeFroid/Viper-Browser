#include "CredentialStore.h"

QDataStream& operator<<(QDataStream &out, const WebCredentials &creds)
{
    out << creds.Host;
    out << creds.LastLogin;
    out << creds.Username;
    out << creds.Password;
    out << creds.FormData;

    return out;
}

QDataStream& operator>>(QDataStream &in, WebCredentials &creds)
{
    in >> creds.Host;
    in >> creds.LastLogin;
    in >> creds.Username;
    in >> creds.Password;
    in >> creds.FormData;

    return in;
}
