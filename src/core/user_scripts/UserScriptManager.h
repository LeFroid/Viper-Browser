#ifndef USERSCRIPTMANAGER_H
#define USERSCRIPTMANAGER_H

#include "Settings.h"
#include "UserScript.h"

#include <memory>
#include <vector>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebEngineScript>

class UserScriptModel;

/**
 * @class UserScriptManager
 * @brief Manages a collection of GreaseMonkey-style user scripts
 */
class UserScriptManager : public QObject
{
    Q_OBJECT
public:
    /// Constructs the user script manager, given a shared pointer to the application settings object and an optional parent object
    explicit UserScriptManager(Settings *settings, QObject *parent = nullptr);

    /// Saves user script information (i.e. which scripts are enabled) before destruction
    virtual ~UserScriptManager();

    /// Sets the state of the user script system. If set to true, scripts will be injected as per their specifications, otherwise scripts will be ignored
    void setEnabled(bool value);

    /// Returns a pointer to the user script model
    UserScriptModel *getModel();

    /**
     * @brief Searches all user scripts for any that match the given URL for injection
     * @param url The URL of the resource being loaded
     * @param injectionTime The time of script injection relative to the initial page load request
     * @param isMainFrame True if the frame associated with the url is the main frame of the page, false if it is a sub-frame
     * @return Concatenated user script data if one or more scripts are found, or an empty string if no scripts match the URL
     */
    QString getScriptsFor(const QUrl &url, ScriptInjectionTime injectionTime, bool isMainFrame);

    /// Returns all of the user scripts associated with the given url
    std::vector<QWebEngineScript> getAllScriptsFor(const QUrl &url);

signals:
    /// Emitted when a user script has been created by the user and can be loaded into the script editor
    void scriptCreated(int scriptIdx);

public slots:
    /**
     * @brief Attempts to download and install the user script from the given URL
     * @param url The location of the user script to be installed
     */
    void installScript(const QUrl &url);

    /**
     * @brief Creates a new user script, given some basic meta-information about its contents
     * @param name Name of the user script
     * @param nameSpace Namespace of the user script
     * @param description Brief description regarding the purpose or functionality of the script
     * @param version Version string of the script
     */
    void createScript(const QString &name, const QString &nameSpace, const QString &description, const QString &version);

private:
    /// Pointer to the user scripts model
    UserScriptModel *m_model;
};

#endif // USERSCRIPTMANAGER_H
