#ifndef FASTHASH_H
#define FASTHASH_H

#include <string>
#include <QtGlobal>

/**
 * @class FastHash
 * @brief A class implementing the rabin-karp string matching algorithm
 *        through a set of static methods. This is used in cases where
 *        a very large number of strings must be compared to a target
 *        as fast as possible
 */
class FastHash
{
public:
    FastHash() = default;

    /// Calculates and returns the difference hash of a needle, which is later used in the rolling hash mechanism
    static quint64 getDifferenceHash(quint64 needleLength);

    /// Calculates and returns the Rabin-Karp hash value of the given needle
    static quint64 getNeedleHash(const std::wstring &needle);

    /**
     * Determines whether or not a string (haystack) contains a substring (needle)
     *
     * @param needle A reference to the target substring
     * @param haystack The string that will be searched for a substring
     * @param needleHash A precomputed hash of the needle
     * @param differenceHash A precomputed difference hash of the needle, which is used to calculate a rolling hash
     * @return True if needle is found in haystack, false if else
     */
    static bool isMatch(const std::wstring &needle, const std::wstring &haystack, quint64 needleHash, quint64 differenceHash);

    /// Radix length, or base used in the rolling hash
    static const quint64 RadixLength;

    /// A fairly large prime modulus used in the rolling hash
    static const quint64 Prime;
};

#endif // FASTHASH_H
