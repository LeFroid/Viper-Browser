#include "CommonUtil.h"
#include "FastHash.h"

const quint64 FastHash::RadixLength = 256ULL;
const quint64 FastHash::Prime = 89999027ULL;

quint64 FastHash::getDifferenceHash(quint64 needleLength)
{
    if (needleLength <= 1)
        return 1;

    quint64 result = 1;
    quint64 exp = static_cast<quint64>(needleLength - 1ULL);
    for (quint64 i = 0; i < exp; ++i)
        result = (result * RadixLength) % Prime;
    return result;
}

quint64 FastHash::getNeedleHash(const std::wstring &needle)
{
    const int needleLength = static_cast<int>(needle.size());
    const wchar_t *needlePtr = needle.c_str();

    quint64 needleHash = 0;

    for (int index = 0; index < needleLength; ++index)
        needleHash = (RadixLength * needleHash + (*(needlePtr + index))) % Prime;

    return needleHash;
}

bool FastHash::isMatch(const std::wstring &needle, const std::wstring &haystack, quint64 needleHash, quint64 differenceHash)
{
    const int needleLength = static_cast<int>(needle.size());
    const int haystackLength = static_cast<int>(haystack.size());

    if (needleLength > haystackLength)
        return false;
    if (needleLength == 0)
        return true;

    const wchar_t *needlePtr = needle.c_str();
    const wchar_t *haystackPtr = haystack.c_str();

    int i, j;
    quint64 t = 0;

    // Calculate the hash value of first substring of the haystack [0, needleLen)
    for (i = 0; i < needleLength; ++i)
        t = (RadixLength * t + (*(haystackPtr + i))) % Prime;

    const int lengthDiff = haystackLength - needleLength;
    for (i = 0; i <= lengthDiff; ++i)
    {
        if (needleHash == t)
        {
            for (j = 0; j < needleLength; j++)
            {
                if (*(haystackPtr + i + j) != *(needlePtr + j))
                    break;
            }

            if (j == needleLength)
                return true;
        }

        if (i < lengthDiff)
        {
            t = RadixLength * (t + Prime - differenceHash * (*(haystackPtr + i)) % Prime) % Prime;
            t = (t + (*(haystackPtr + needleLength + i))) % Prime;
            // alternative form:
            //t = ((RadixLength * ((t + Prime - differenceHash * haystackPtr[i] % Prime) % Prime) % Prime) + haystackPtr[needleLength + i]) % Prime;
        }
    }

    return false;
}
