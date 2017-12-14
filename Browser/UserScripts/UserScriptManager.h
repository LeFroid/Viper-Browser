#ifndef USERSCRIPTMANAGER_H
#define USERSCRIPTMANAGER_H

#include "Settings.h"
#include "UserScript.h"

#include <memory>
#include <vector>
#include <QObject>
#include <QString>
#include <QUrl>

/**
 * @class UserScriptManager
 * @brief Manages a collection of GreaseMonkey-style user scripts
 */
class UserScriptManager : public QObject
{
    Q_OBJECT
public:
    /// Constructs the user script manager, given a shared pointer to the application settings object and an optional parent object
    explicit UserScriptManager(std::shared_ptr<Settings>, QObject *parent = nullptr);

    /// Searches all user scripts for any that match the given url, returning the concatenated script data if
    /// any are found, or an empty QString if no scripts match
    QString getScriptsFor(const QUrl &url, bool isMainFrame);

private:
    /// Loads user script files from the user script directory, into the script container
    void load();

    /// Loads dependencies for the user script in the container at the given index
    void loadDependencies(int scriptIdx);

    /// Saves user script information to a user script configuration file
    void save();

private:
    /// Application settings
    std::shared_ptr<Settings> m_settings;

    /// Container of user scripts
    std::vector<UserScript> m_scripts;

    /// True if user scripts are enabled, false if else
    bool m_enabled;

    /// User script template file contents
    QString m_scriptTemplate;

    /// Absolute path to user script dependency directory
    QString m_scriptDepDir;
};

#endif // USERSCRIPTMANAGER_H
