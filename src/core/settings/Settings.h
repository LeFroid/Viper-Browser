#ifndef SETTINGS_H
#define SETTINGS_H

#include "BrowserSetting.h"

#include <QObject>
#include <QMap>
#include <QSettings>

class QWebEngineSettings;

/// The types of pages that can be loaded by default when a new web page or tab is created
enum class NewTabType
{
    /// User-specified home page URL
    HomePage      = 0,

    /// Empty page
    BlankPage     = 1,

    /// Page with card layout of most frequently visited and/or pinned web pages
    FavoritesPage = 2
};

/**
 * @class Settings
 * @brief Used to access and modify configurable settings of the browser
 */
class Settings : public QObject
{
    Q_OBJECT

    /// Settings version
    const static QString Version;

public:
    /// Settings constructor - loads browser settings and sets to defaults if applicable
    explicit Settings(QWebEngineSettings *webSettings);

    /// Returns the path to the item associated with the path- or file-related key
    QString getPathValue(BrowserSetting key);

    /// Returns the value associated with the given key
    QVariant getValue(BrowserSetting key);

    /// Sets the value for the given key
    void setValue(BrowserSetting key, const QVariant &value);

    /// Returns true if the settings have been created in this session, false if else
    bool firstRun() const;

Q_SIGNALS:
    /// Emitted whenever a setting is changed to the given value
    void settingChanged(BrowserSetting setting, const QVariant &value);

private:
    /// Sets the default browser settings
    void setDefaults();

    /// Updates the settings after a version change
    void updateSettings();

private:
    /// True if the settings have been created in this session, false if otherwise
    bool m_firstRun;

    /// QSettings object used to access configuration
    QSettings m_settings;

    /// Storage path from settings
    QString m_storagePath;

    /// Mapping of \ref BrowserSetting values to their equivalent key names
    QMap<BrowserSetting, QString> m_settingMap;

    /// Default profile web settings
    QWebEngineSettings *m_webSettings;
};

#endif // SETTINGS_H
