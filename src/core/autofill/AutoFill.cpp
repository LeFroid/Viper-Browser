#include "AutoFill.h"
#include "AutoFillDialog.h"
#include "BrowserApplication.h"
#include "Settings.h"
#include "WebPage.h"

#include <type_traits>
#include <vector>

#include <QFile>
#include <QWebEngineScript>
#include <QtGlobal>
#include <QDebug>

AutoFill::AutoFill(Settings *settings) :
    QObject(nullptr),
    m_credentialStore(nullptr),
    m_formFillScript(),
    m_enabled(settings->getValue(BrowserSetting::EnableAutoFill).toBool())
{
    setObjectName(QStringLiteral("AutoFill"));

    QFile scriptFile(QStringLiteral(":/AutoFill.js"));
    if (scriptFile.open(QIODevice::ReadOnly))
        m_formFillScript = scriptFile.readAll();
    scriptFile.close();

    connect(settings, &Settings::settingChanged, this, &AutoFill::onSettingChanged);
    connect(sBrowserApplication, &BrowserApplication::pluginsLoaded, this, &AutoFill::onPluginsLoaded);
}

AutoFill::~AutoFill()
{
}

void AutoFill::onFormSubmitted(WebPage *page, const QString &pageUrl, const QString &username, const QString &password, const QMap<QString, QVariant> &formData)
{
    if (!m_credentialStore || !m_enabled)
        return;

    if (!page)
        return;

    QWidget *parentWidget = qobject_cast<QWidget*>(page->parent());
    if (!parentWidget)
        return;

    // Todo: check if URL is blocked from inclusion in auto fill system

    // Fetch or create a new set of credentials associated with the login
    bool isExisting = false;
    WebCredentials credentials;
    std::vector<WebCredentials> savedLogins = m_credentialStore->getCredentialsFor(pageUrl);
    for (const WebCredentials &creds : savedLogins)
    {
        if (creds.Username.compare(username) == 0)
        {
            isExisting = true;
            credentials = creds;
            credentials.LastLogin = QDateTime::currentDateTime();
            // Update last login time in data store. If password has changed, a dialog will appear
            // to ask the user whether or not they want it to be updated.
            m_credentialStore->updateCredentials(credentials);

            if (creds.Password.compare(password) == 0)
                return;

            break;
        }
    }

    credentials.Host = QUrl::fromUserInput(pageUrl).host();
    credentials.Username = username;
    credentials.Password = password;
    credentials.LastLogin = QDateTime::currentDateTime();

    for (auto it = formData.cbegin(); it != formData.cend(); ++it)
        credentials.FormData[it.key()] = it.value().toString();

    QWidget *window = parentWidget->window();
    AutoFillDialog *dialog = new AutoFillDialog;

    AutoFillDialog::DialogAction action = isExisting ? AutoFillDialog::UpdateCredentialsAction : AutoFillDialog::NewCredentialsAction;
    dialog->setAction(action);
    dialog->setInformation(credentials);

    connect(dialog, &AutoFillDialog::rejected, dialog, &AutoFillDialog::deleteLater);
    connect(dialog, &AutoFillDialog::saveCredentials,   this, &AutoFill::saveCredentials);
    connect(dialog, &AutoFillDialog::updateCredentials, this, &AutoFill::updateCredentials);
    connect(dialog, &AutoFillDialog::removeCredentials, this, &AutoFill::removeCredentials);

    dialog->alignAndShow(window->frameGeometry());
}

void AutoFill::onPageLoaded(bool ok)
{
    if (!ok || !m_credentialStore || !m_enabled)
        return;

    WebPage *page = qobject_cast<WebPage*>(sender());
    if (!page)
        return;

    std::vector<WebCredentials> savedLogins = m_credentialStore->getCredentialsFor(page->url());
    if (savedLogins.empty())
        return;

    const WebCredentials &lastUsedCreds = savedLogins.at(0);

    QString scriptData;

    for (auto it = lastUsedCreds.FormData.cbegin(); it != lastUsedCreds.FormData.cend(); ++it)
        scriptData.append(QString("autoFillVals['%1'] = '%2';\n").arg(it.key(), it.value()));

    page->runJavaScript(m_formFillScript.arg(scriptData), QWebEngineScript::ApplicationWorld);
}

std::vector<WebCredentials> AutoFill::getAllCredentials()
{
    std::vector<WebCredentials> result;

    if (!m_credentialStore)
        return result;

    std::vector<QString> savedHosts = m_credentialStore->getHostNames();
    if (savedHosts.empty())
        return result;

    for (const QString &host : savedHosts)
    {
        QUrl url = QUrl::fromUserInput(host);
        std::vector<WebCredentials> temp = m_credentialStore->getCredentialsFor(url);

        result.reserve(result.size() + temp.size());
        result.insert(result.end(), temp.begin(), temp.end());
    }

    return result;
}

void AutoFill::saveCredentials(const WebCredentials &credentials)
{
    if (m_credentialStore)
        m_credentialStore->addCredentials(credentials);
}

void AutoFill::updateCredentials(const WebCredentials &credentials)
{
    if (m_credentialStore)
        m_credentialStore->updateCredentials(credentials);
}

void AutoFill::removeCredentials(const WebCredentials &credentials)
{
    if (m_credentialStore)
        m_credentialStore->removeCredentials(credentials);
}

void AutoFill::onSettingChanged(BrowserSetting setting, const QVariant &value)
{
    if (setting == BrowserSetting::EnableAutoFill)
        m_enabled = value.toBool();
}

void AutoFill::onPluginsLoaded()
{
    // Use platform-specific plugins for Mac (Q_OS_MACOS), Windows (Q_OS_WIN), *nix
    // For *nix platforms, use kwallet or libsecret, depending on user's environment
#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
    const bool kdeIndicator = qEnvironmentVariableIsSet("KDE_SESSION_VERSION")
            || (qEnvironmentVariable("XDG_CURRENT_DESKTOP", QString()).compare(QStringLiteral("KDE")) == 0);
    const QString providerName = kdeIndicator ? QStringLiteral("CredentialStoreKWallet") : QStringLiteral("CredentialStoreSecret");
    QObject *provider = sBrowserApplication->getService(providerName);
#else
    QObject *provider = nullptr;
#endif

    if (CredentialStore *store = qobject_cast<CredentialStore*>(provider))
        m_credentialStore = store;
    else
        qWarning() << "No credential store found. Disabling AutoFill.";
}
