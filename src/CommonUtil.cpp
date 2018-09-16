#include "CommonUtil.h"

#include <QBuffer>

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

    QIcon iconFromBase64(QByteArray data)
    {
        QByteArray decoded = QByteArray::fromBase64(data);

        QBuffer buffer(&decoded);
        QImage img;
        img.load(&buffer, "PNG");

        return QIcon(QPixmap::fromImage(img));
    }

    QByteArray iconToBase64(QIcon icon)
    {
        // First convert the icon into a QImage, and from that place data into a byte array
        QImage img = icon.pixmap(32, 32).toImage();

        QByteArray data;
        QBuffer buffer(&data);
        img.save(&buffer, "PNG");

        return data.toBase64();
    }
}
