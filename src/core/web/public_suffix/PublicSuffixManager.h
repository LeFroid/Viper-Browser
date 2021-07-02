#ifndef _PUBLIC_SUFFIX_MANAGER_H_
#define _PUBLIC_SUFFIX_MANAGER_H_

#include "public_suffix/PublicSuffixTree.h"
#include <QObject>

/**
 * @class PublicSuffixManager
 * @brief Acts as the interface between the public suffix database and
 *        any application APIs that rely on such information (eg, the URL API)
 */
class PublicSuffixManager : public QObject
{
public:
    /// Constructs the public suffix manager
    explicit PublicSuffixManager(QObject *parent = nullptr);

    /// Destructor
    ~PublicSuffixManager();

    /// Singleton instance
    static PublicSuffixManager &instance();

    /**
     * @brief findTld Searches the public suffix list for the top-level domain that the given domain belongs to
     * @param domain Domain with which to perform the search
     * @return Substring of the domain, containing the top-level domain segment
     */
    QString findTld(const QString &domain) const;

    /**
     * @brief findRule Searches for a public suffix rule that applies to the given domain.
     * @param domain Punycode-formatted domain string
     * @return Public suffix rule, if found, otherwise a "null" or no-op rule is returned
     */
    PublicSuffixRule findRule(const QString &domain) const;

private:
    /// Loads the public suffix data file into memory, and constructs the tree out of the
    /// public suffix rules
    void load();

private:
    /// Tree of suffix rules
    PublicSuffixTree m_tree;
};

#endif // _PUBLIC_SUFFIX_MANAGER_H_
