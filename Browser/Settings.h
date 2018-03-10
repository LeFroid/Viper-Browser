#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>

/**
 * @class Settings
 * @brief Used to access and modify configurable settings of the browser
 */
class Settings
{
public:
    /// Settings constructor - loads browser settings and sets to default if applicable
    Settings();

    /// Returns the path to the item associated with the *path-related* key as a string
    /// Ex: BookmarkPath, CookiePath, etc.
    QString getPathValue(const QString &key);

    /// Returns the value associated with the given key
    QVariant getValue(const QString &key);

    /// Sets the value for the given key
    void setValue(const QString &key, const QVariant &value);

    /// Returns true if the settings have been created in this session, false if else
    bool firstRun() const;

    /// Applies the web engine settings to the QWebSettings instance
    void applyWebSettings();

private:
    /// Sets the default browser settings
    void setDefaults();

private:
    /// True if the settings have been created in this session, false if otherwise
    bool m_firstRun;

    /// QSettings object used to access configuration
    QSettings m_settings;

    /// Storage path from settings
    QString m_storagePath;
};

#endif // SETTINGS_H
