#ifndef COMMONUTIL_H
#define COMMONUTIL_H

#include <QString>
#include <QtGlobal>

/// Collection of common utility functions
namespace CommonUtil
{
    /// Converts the given number of bytes into a user-readable string (ex: "10 MB", "1.2 GB" etc)
    QString bytesToUserFriendlyStr(quint64 amount);
}

#endif // COMMONUTIL_H
