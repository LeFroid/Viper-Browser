#ifndef USERAGENTMANAGER_H
#define USERAGENTMANAGER_H

#include <memory>

#include <QMap>
#include <QObject>

class AddUserAgentDialog;
class Settings;

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
public:
    /// Constructs the user agent manager, which will load custom user agents from a JSON data file
    explicit UserAgentManager(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    /// User agent manager destructor - saves user agents to data file
    ~UserAgentManager();

    /// Returns the category of the active user agent string, or an empty string if the default user agent is in use
    const QString &getUserAgentCategory() const;

    /// Returns the active user agent, or an empty user agent structure if the default is in use
    const UserAgent &getUserAgent() const;

    /// Sets the active user agent
    void setActiveAgent(const QString &category, const UserAgent &agent);

    /// Returns a const_iterator to the first item in the custom user agent map
    QMap< QString, QList<UserAgent> >::const_iterator getAgentIterBegin() const { return m_userAgents.cbegin(); }

    /// Returns a const_iterator to the end of the custom user agent map
    QMap< QString, QList<UserAgent> >::const_iterator getAgentIterEnd() const { return m_userAgents.cend(); }

signals:
    /// Emitted when the map of user agents has changed, so that any UI elements using user agent information may be updated
    void updateUserAgents();

public slots:
    /// Shows the "Add an User Agent" dialog
    void addUserAgent();

    /// Disables the currently active user agent
    void disableActiveAgent();

    /// Shows the window used for modifying user agents
    void modifyUserAgents();

private slots:
    /// Called when the "Add an User Agent" dialog has been closed and the user confirms the addition of a new agent
    void onUserAgentAdded();

private:
    /// Loads user agents from a data file
    void load();

    /// Saves user agents to a data file
    void save();

private:
    /// Application settings
    std::shared_ptr<Settings> m_settings;

    /// Map of custom user agents. Key is the category name, value is a list of user agents
    QMap< QString, QList<UserAgent> > m_userAgents;

    /// Category of the active user agent string. Empty if default user agent is in use
    QString m_activeAgentCategory;

    /// Current user agent. Empty if the default user agent is in use
    UserAgent m_activeAgent;

    /// Dialog used to add new user agents
    AddUserAgentDialog *m_addAgentDialog;
};

#endif // USERAGENTMANAGER_H
