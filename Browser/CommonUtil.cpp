#include "CommonUtil.h"

namespace CommonUtil
{
    QString bytesToUserFriendlyStr(quint64 amount)
    {
        QString userStr;
        double valDiv = amount;

        if (amount >= 1099511627776)
        {
            // >= 1 TB
            valDiv /= 1099511627776;
            userStr = QString::number(valDiv, 'f', 2) + " TB";
        }
        else if (amount >= 1073741824)
        {
            // >= 1 GB
            valDiv /= 1073741824;
            userStr = QString::number(valDiv, 'f', 2) + " GB";
        }
        else if (amount >= 1048576)
        {
            // >= 1 MB
            valDiv /= 1048576;
            userStr = QString::number(valDiv, 'f', 2) + " MB";
        }
        else if (amount > 1024)
        {
            // >= 1 KB
            valDiv /= 1024;
            userStr = QString::number(valDiv, 'f', 2) + " KB";
        }
        else
        {
            // < 1 KB
            return QString::number(amount) + " bytes";
        }

        return userStr;
    }
}
