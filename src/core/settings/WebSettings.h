#ifndef WEBSETTINGS_H
#define WEBSETTINGS_H

#include "BrowserSetting.h"
#include "ISettingsObserver.h"
#include "ServiceLocator.h"

#include <QObject>
#include <QVariant>

class CookieJar;
class Settings;
class UserAgentManager;
class QWebEngineProfile;
class QWebEngineSettings;

/**
 * @class WebSettings
 * @brief Acts as an intermediary between web-related settings that are
 *        specified in the \ref Settings class, and the settings management
 *        system of the web engine module
 */
class WebSettings : public QObject, ISettingsObserver
{
    Q_OBJECT

public:
    /// Constructs the web settings given a reference to the service locator, a pointer to the web engine settings
    /// that will be managed by this object, and also a pointer to the web engine profile which contains other web-related
    /// variables
    explicit WebSettings(const ViperServiceLocator &serviceLocator, QWebEngineSettings *webEngineSettings, QWebEngineProfile *webEngineProfile);

    /// Destructor
    ~WebSettings();

private slots:
    /// Listens for any settings changes that affect the web configuration
    void onSettingChanged(BrowserSetting setting, const QVariant &value) override;

private:
    /// Loads the web-related settigns during instantiation of this object into the default web profile
    void init(Settings *appSettings);

private:
    /// Points to the application cookie jar
    CookieJar *m_cookieJar;

    /// Points to the browser user agent manager
    UserAgentManager *m_userAgentManager;

    /// Web engine settings for the web profile that was passed in the constructor
    QWebEngineSettings *m_webEngineSettings;

    /// Web engine profile being managed by this object
    QWebEngineProfile *m_webEngineProfile;
};

#endif // WEBSETTINGS_H
