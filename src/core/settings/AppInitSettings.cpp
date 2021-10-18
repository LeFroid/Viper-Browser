#include "AppInitSettings.h"

#include <stdlib.h>

#if defined(__unix__)
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <unistd.h>
#elif defined(_WIN32) || defined(WIN32) || defined(_WIN64)
#  include <direct.h>
#  include <windows.h>
#endif

#include <algorithm>
#include <fstream>
#include <iterator>

AppInitSettings::AppInitSettings() :
    m_fileName(),
    m_settings(),
    m_settingKeyMap {
        { AppInitKey::ProcessModel,          R"(ProcessModel)" },
        { AppInitKey::DisableGPU,            R"(DisableGPU)"   },
        { AppInitKey::CompletedInitialSetup, R"(CompletedInitialSetup)" }
    }
{
    load();
}

bool AppInitSettings::hasSetting(AppInitKey key) const
{
    std::string keyString = m_settingKeyMap.at(key);
    return m_settings.find(keyString) != m_settings.end();
}

void AppInitSettings::removeSetting(AppInitKey key)
{
    std::string keyString = m_settingKeyMap.at(key);
    m_settings.erase(keyString);
    save();
}

std::string AppInitSettings::getValue(AppInitKey key) const
{
    std::string keyString = m_settingKeyMap.at(key);
    auto it = m_settings.find(keyString);
    if (it != m_settings.end())
        return it->second;

    return std::string();
}

void AppInitSettings::setValue(AppInitKey key, const std::string &value)
{
    std::string keyString = m_settingKeyMap.at(key);
    m_settings[keyString] = value;
    save();
}

void AppInitSettings::load()
{
    // Create settings directory if not already done so
#if defined(__unix__)
    std::string homeDir = getenv("HOME");
    std::string settingsDir = homeDir + "/.config/Vaccarelli";
    struct stat st;
    bool ok = stat(settingsDir.c_str(), &st) == 0;
    if (!ok || (st.st_mode & S_IFDIR) == 0)
    {
        if (mkdir(settingsDir.c_str(), 0777) != 0)
            return;
    }

    m_fileName = settingsDir + "/init-flags.cfg";
#elif defined(_WIN32) || defined(WIN32) || defined(_WIN64)
    std::string dataDir = getenv("APPDATA");
    std::string settingsDir = dataDir + "\\Local\\Vaccarelli";
    DWORD settingsDirType = GetFileAttributesA(settingsDir.c_str());
    if (settingsDirType == INVALID_FILE_ATTRIBUTES)
        return;
    if ((settingsDirType & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        if (_mkdir(settingsDir.c_str()) != 0)
            return;
    }

    m_fileName = settingsDir + "\\init-flags.cfg";
#endif

    if (m_fileName.empty())
        return;

    std::string line;
    std::ifstream handle(m_fileName);
    if (!handle.is_open())
        return;

    while (std::getline(handle, line))
    {
        // split line by '=' delimiter, strip whitespaces, and add setting to the map
        size_t delimPos = line.find('=', 0);
        if (delimPos == std::string::npos)
            continue;

        std::string key = line.substr(0, delimPos);
        std::string value = line.substr(delimPos + 1);

        // Strip whitespaces from the key but not from the value (unless value starts with whitespaces)
        std::string keyNoWhitepace;
        std::remove_copy(key.begin(), key.end(), std::back_inserter(keyNoWhitepace), ' ');

        size_t firstNonWhitespace = value.find_first_not_of(' ', 0);
        if (firstNonWhitespace != std::string::npos && firstNonWhitespace > 0)
            value = value.substr(firstNonWhitespace);

        m_settings[keyNoWhitepace] = value;
    }
}

void AppInitSettings::save()
{
    if (m_fileName.empty())
        return;

    std::ofstream handle(m_fileName);
    if (!handle.is_open())
        return;

    std::string newLine = "\n";
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64)
    newLine = "\r\n";
#endif

    for (const auto &it : m_settings)
    {
        handle << it.first << '=' << it.second << newLine;
    }

    handle.close();
}
