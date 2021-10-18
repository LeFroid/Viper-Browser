#ifndef APPINITSETTINGS_H
#define APPINITSETTINGS_H

#include <functional>
#include <string>
#include <unordered_map>

/// Enumeration of the valid application initialization settings
enum class AppInitKey : int
{
    /// Web Engine process model
    ProcessModel,

    /// Flag determining whether or not to disable GPU hardware acceleration
    DisableGPU,

    /// Flag representing whether or not the user has set their initial profile settings
    /// through an interactive dialog
    CompletedInitialSetup
};

/// Calculates the hash value of the \ref AppInitKey so it can be stored in a hash map
struct AppInitKeyHash
{
    template <typename T>
    std::size_t operator()(const T &t) const noexcept
    {
        return std::hash<int>{}(static_cast<int>(t));
    }
};

/**
 * @class AppInitSettings
 * @brief Manages application settings that must be loaded prior to instantiating the
 *        \ref BrowserApplication. Works with string values only, as this class has
 *        only the basic purpose of passing command line arguments to the application.
 */
class AppInitSettings
{
public:
    /// Constructs the application initialization settings handler, which loads
    /// any flags set in the pre-init config file
    AppInitSettings();

    /// Returns true if the given key is in the settings, false if it is unset
    bool hasSetting(AppInitKey key) const;

    /// Removes the setting with the given key from the map, if it was found
    void removeSetting(AppInitKey key);

    /// Returns the value associated with the given key, or an empty string if the key is not set
    std::string getValue(AppInitKey key) const;

    /// Sets the value of the given setting type
    void setValue(AppInitKey key, const std::string &value);

private:
    /// Loads any flags that are set in the pre-app init config file
    void load();

    /// Saves the current settings to the pre-app init config file
    void save();

private:
    /// Full name of the settings file, including the path information
    /// Examples: /home/user/.config/Vaccarelli/init-flags.cfg and C:\User\AppData\Local\Vaccarelli\init-flags.cfg
    std::string m_fileName;

    /// Map of key-value pairs in the settings
    std::unordered_map<std::string, std::string> m_settings;

    /// Map of valid keys to their corresponding string values
    std::unordered_map<AppInitKey, std::string, AppInitKeyHash> m_settingKeyMap;
};

#endif // APPINITSETTINGS_H
