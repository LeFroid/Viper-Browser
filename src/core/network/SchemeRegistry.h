#ifndef SCHEMEREGISTRY_H
#define SCHEMEREGISTRY_H

#include <array>
#include <QString>

/**
 * @class SchemeRegistry
 * @brief Defines parsing parameters and security definitions of custom network schemes,
 *        which are then registered with the web engine.
 */
class SchemeRegistry
{
public:
    /// Registers all custom scheme information with the web engine.
    /// This method must be called before the \ref BrowserApplication class is initialized
    static void registerSchemes();

    /**
     * @brief Determines whether or not the given scheme is secure
     * @param scheme The scheme (ex: http, ftp, file)
     * @return True if the scheme is secure, false otherwise
     */
    static bool isSecure(const QString &scheme);

    /// Returns true if the given scheme should be whitelisted by the advertisement blocking system
    static bool isSchemeWhitelisted(const QString &scheme);

private:
    /// Array of secure schemes
    static const std::array<QString, 2> SecureSchemes;

    /// Array of whitelisted schemes
    static const std::array<QString, 4> WhitelistedSchemes;
};

#endif // SCHEMEREGISTRY_H
