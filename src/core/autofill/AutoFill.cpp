#include "AutoFill.h"
#include "AutoFillDialog.h"
#include "BrowserApplication.h"
#include "CredentialStoreImpl.h"
#include "Settings.h"
#include "WebPage.h"

#include <type_traits>
#include <vector>

#include <QFile>
#include <QWebEngineScript>

AutoFill::AutoFill(QObject *parent) :
    QObject(parent),
    m_credentialStore(nullptr),
    m_formFillScript()
{
    if (!std::is_same<CredentialStoreImpl, std::nullptr_t>::value)
        m_credentialStore = std::make_unique<CredentialStoreImpl>();

    QFile scriptFile(QLatin1String(":/AutoFill.js"));
    if (scriptFile.open(QIODevice::ReadOnly))
        m_formFillScript = scriptFile.readAll();
    scriptFile.close();
}

AutoFill::~AutoFill()
{
}

void AutoFill::onFormSubmitted(WebPage *page, const QString &pageUrl, const QString &username, const QString &password, const QMap<QString, QVariant> &formData)
{
    if (!m_credentialStore || !sBrowserApplication->getSettings()->getValue(BrowserSetting::EnableAutoFill).toBool())
        return;

    if (!page || !page->view())
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

    QWidget *window = page->view()->window();
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

void AutoFill::onPageLoaded(WebPage *page, const QUrl &url)
{
    if (!m_credentialStore || !sBrowserApplication->getSettings()->getValue(BrowserSetting::EnableAutoFill).toBool())
        return;

    std::vector<WebCredentials> savedLogins = m_credentialStore->getCredentialsFor(url);
    if (savedLogins.empty())
        return;

    const WebCredentials &lastUsedCreds = savedLogins.at(0);

    QString scriptData;

    for (auto it = lastUsedCreds.FormData.cbegin(); it != lastUsedCreds.FormData.cend(); ++it)
        scriptData.append(QString("autoFillVals['%1'] = '%2';\n").arg(it.key()).arg(it.value()));

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
