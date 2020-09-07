#ifndef AUTOFILL_H
#define AUTOFILL_H

#include "CredentialStore.h"
#include "Settings.h"
#include "ISettingsObserver.h"

#include <memory>

#include <QMap>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVariant>

class WebPage;

/**
 * @class AutoFill
 * @brief Handles automatic filling of form data that the user allows
 *        the system to manage for them.
 */
class AutoFill : public QObject, public ISettingsObserver
{
    friend class AutoFillCredentialsView;

    Q_OBJECT

public:
    /// Constructs the AutoFill manager given a pointer to the application settings
    /// AutoFill constructor
    explicit AutoFill(Settings *settings);

    /// Destructor
    ~AutoFill();

    /**
     * @brief Called by \ref AutoFillBridge when a form was submitted on a \ref WebPage
     * @param page Pointer to the web page that triggered the event
     * @param pageUrl The URL of the page containing the form
     * @param username The username field of the form
     * @param password The password entered by the user
     * @param formData All of the information included in the form, as a urlencoded byte array
     */
    void onFormSubmitted(WebPage *page, const QString &pageUrl, const QString &username, const QString &password, const QMap<QString, QVariant> &formData);

public Q_SLOTS:
    /// Checks for any saved login information for the URL associated with the web page
    /// that triggered this slot, completing any forms found on the page if auto fill is enabled
    void onPageLoaded(bool ok);

protected:
    /// Returns a list of all credentials that are stored in the system
    std::vector<WebCredentials> getAllCredentials();

protected Q_SLOTS:
    /// Removes the given credentials from the \ref CredentialStore
    void removeCredentials(const WebCredentials &credentials);

private Q_SLOTS:
    /// Saves the given credentials in the \ref CredentialStore
    void saveCredentials(const WebCredentials &credentials);

    /// Updates the given credentials in the \ref CredentialStore
    void updateCredentials(const WebCredentials &credentials);

    /// Listens for any settings changes that affect the AutoFill system (ex: enable/disable autofill)
    void onSettingChanged(BrowserSetting setting, const QVariant &value) override;

    /// Attempts to load a credential store, after the application sends a plugin notification signal
    void onPluginsLoaded();

private:
    /// Credential storage system
    CredentialStore *m_credentialStore;
    //std::unique_ptr<CredentialStore> m_credentialStore;

    /// Form filling javascript template
    QString m_formFillScript;

    /// Flag indicating whether or not the AutoFill system is enabled
    bool m_enabled;
};


#endif // AUTOFILL_H
