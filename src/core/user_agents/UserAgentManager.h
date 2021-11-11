#ifndef USERAGENTMANAGER_H
#define USERAGENTMANAGER_H

#include <memory>

#include <QMap>
#include <QObject>
#include <vector>

class AddUserAgentDialog;
class Settings;
class UserAgentsWindow;
class QWebEngineProfile;

/// Represents a custom user agent
struct UserAgent
{
    /// Name of the user agent
    QString Name;

    /// Contents of the user agent string
    QString Value;
};

/**
 * @class UserAgentManager
 * @brief Manages the storage and use of custom user agents
 */
class UserAgentManager : public QObject
{
    Q_OBJECT

    friend class UserAgentsWindow;

public:
    using iterator = QMap<QString,std::vector<UserAgent>>::iterator;
    using const_iterator = QMap<QString,std::vector<UserAgent>>::const_iterator;

    /// Constructs the user agent manager, which will load custom user agents from a JSON data file
    explicit UserAgentManager(Settings *settings, QWebEngineProfile *profile, QObject *parent = nullptr);

    /// User agent manager destructor - saves user agents to data file
    ~UserAgentManager();

    /// Returns the category of the active user agent string, or an empty string if the default user agent is in use
    const QString &getUserAgentCategory() const;

    /// Returns the active user agent, or an empty user agent structure if the default is in use
    const UserAgent &getUserAgent() const;

    /// Sets the active user agent
    void setActiveAgent(const QString &category, const UserAgent &agent);

    /// Returns a const_iterator to the first item in the custom user agent map
    const_iterator begin() const { return m_userAgents.cbegin(); }

    /// Returns a const_iterator to the end of the custom user agent map
    const_iterator end() const { return m_userAgents.cend(); }

protected:
    /// Clears all user agents from the map
    void clearUserAgents();

    /// Adds a category of user agents to the map with a given container of agents to be associated with the map
    void addUserAgents(const QString &category, std::vector<UserAgent> &&userAgents);

    /// Called when the \ref UserAgentsWindow is finished saving changes to user agent values, so that the updateUserAgents() signal may be emitted
    void modifyWindowFinished();

Q_SIGNALS:
    /// Emitted when the map of user agents has changed, so that any UI elements using user agent information may be updated
    void updateUserAgents();

public Q_SLOTS:
    /// Shows the "Add an User Agent" dialog
    void addUserAgent();

    /// Disables the currently active user agent
    void disableActiveAgent();

    /// Shows the window used for modifying user agents
    void modifyUserAgents();

private Q_SLOTS:
    /// Called when the "Add an User Agent" dialog has been closed and the user confirms the addition of a new agent
    void onUserAgentAdded();

private:
    /// Loads user agents from a data file
    void load();

    /// Saves user agents to a data file
    void save();

private:
    /// Application settings
    Settings *m_settings;

    /// Map of custom user agents. Key is the category name, value is a list of user agents
    QMap< QString, std::vector<UserAgent> > m_userAgents;

    /// Category of the active user agent string. Empty if default user agent is in use
    QString m_activeAgentCategory;

    /// Current user agent. Empty if the default user agent is in use
    UserAgent m_activeAgent;

    /// Dialog used to add new user agents
    AddUserAgentDialog *m_addAgentDialog;

    /// User agent management window
    UserAgentsWindow *m_userAgentsWindow;

    /// Default web profile
    QWebEngineProfile *m_profile;
};

#endif // USERAGENTMANAGER_H
