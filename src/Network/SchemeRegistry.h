#ifndef SCHEMEREGISTRY_H
#define SCHEMEREGISTRY_H

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
};

#endif // SCHEMEREGISTRY_H
