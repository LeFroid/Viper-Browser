#include "CommonUtil.h"

#include <array>
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

    quint64 quPow(quint64 base, quint64 exp)
    {
        quint64 result = 1;
        while (exp)
        {
            if (exp & 1)
                result *= base;
            exp >>= 1;
            base *= base;
        }
        return result;
    }

    QRegularExpression getRegExpForMatchPattern(const QString &str)
    {
        QChar kleeneStar = QLatin1Char('*');
        int pos = str.indexOf("://");

        // If string is empty or does not specify scheme, return regex accepting anything with http or https scheme
        if (str.isEmpty() || pos <= 0)
            return QRegularExpression(QLatin1String("http(s?)://"));
        else if (str.compare(QLatin1String("<all_urls>")) == 0)
            return QRegularExpression(QLatin1String("(http(s?)|ftp|file)://.*"));

        // Get scheme in the form of a regular expression
        QString converted = str.left(pos);
        if (converted.compare(kleeneStar) == 0)
            converted = QLatin1String("http(s?)");

        if (!converted.startsWith(QLatin1String("http"))
                && !converted.startsWith(QLatin1String("ftp"))
                && !converted.startsWith(QLatin1String("file")))
            return QRegularExpression(QLatin1String("http(s?)://"));

        converted.append(QLatin1String("://"));

        // Get host in the form of a regular expression
        int pathPos = str.indexOf(QLatin1Char('/'), pos + 3);
        if (pathPos < 0)
        {
            // Path character is mandatory
            return QRegularExpression(QLatin1String("http(s?)://"));
        }
        else if (pathPos > 0)
        {
            QString host = str.left(pathPos);
            host = host.mid(pos + 3);

            int lastStarPos = host.lastIndexOf(kleeneStar);
            if (lastStarPos > 0)
                return QRegularExpression(QLatin1String("http(s?)://"));

            if (lastStarPos == 0)
            {
                if (host.size() == 1)
                    converted.append(QLatin1String(".*/"));
                else if (host.at(1) == QLatin1Char('.'))
                {
                    converted.append(QLatin1String(".*"));
                    host = host.mid(1);
                }
                else
                    return QRegularExpression(QLatin1String("http(s?)://"));

            }

            converted.append(host);
        }

        converted.append(QLatin1Char('/'));

        // Get path in the form of a regular expression
        if (pathPos + 1 == str.size())
            return QRegularExpression(converted);

        QString path = str.mid(pathPos + 1);
        path.replace(QLatin1String("*"), QLatin1String(".*"));
        converted.append(path);
        return QRegularExpression(converted);
    }

    bool doUrlsMatch(const QUrl &a, const QUrl &b, bool ignoreScheme)
    {
        QString aString = a.toString().toLower(), bString = b.toString().toLower();

        if (ignoreScheme)
        {
            QRegularExpression schemeExpr{QLatin1String("^[a-zA-Z]+://")};
            aString.remove(schemeExpr);
            bString.remove(schemeExpr);
        }

        QRegularExpression userInfoExpr{QLatin1String("^.*:.*@")};
        aString.remove(userInfoExpr);
        bString.remove(userInfoExpr);

        QRegularExpression wwwExpr{QLatin1String("^www\\.")};
        aString.remove(wwwExpr);
        bString.remove(wwwExpr);

        if (aString.endsWith(QLatin1Char('/')))
            aString = aString.left(aString.size() - 1);
        if (bString.endsWith(QLatin1Char('/')))
            bString = bString.left(bString.size() - 1);

        return (aString.compare(bString) == 0);
    }

    QStringList tokenizePossibleUrl(QString str)
    {
        str = str.replace(QRegularExpression(QLatin1String("[\\?=&\\./:-]+")), QLatin1String(" "));

        const std::array<QRegularExpression, 2> delimExpressions {
            QRegularExpression(QLatin1String("[A-Z]{1}[0-9]{1}")),
            QRegularExpression(QLatin1String("[0-9]{1}[A-Z]{1}"))
        };

        for (const QRegularExpression &expr : delimExpressions)
        {
            int matchPos = 0;
            auto match = expr.match(str, matchPos);
            while (match.hasMatch())
            {
                matchPos = match.capturedStart();
                str.insert(matchPos + 1, QLatin1Char(' '));
                match = expr.match(str, matchPos + 2);
            }
        }

        return str.split(QLatin1Char(' '), QStringSplitFlag::SkipEmptyParts);
    }
}
