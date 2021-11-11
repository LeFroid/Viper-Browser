#include "UserAgentManager.h"
#include "AddUserAgentDialog.h"
#include "Settings.h"
#include "UserAgentsWindow.h"

#include <QByteArray>
#include <QFile>
#include <QTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QWebEngineProfile>

UserAgentManager::UserAgentManager(Settings *settings, QWebEngineProfile *profile, QObject *parent) :
    QObject(parent),
    m_settings(settings),
    m_userAgents(),
    m_activeAgentCategory(),
    m_activeAgent(),
    m_addAgentDialog(nullptr),
    m_userAgentsWindow(nullptr),
    m_profile(profile)
{
    setObjectName(QLatin1String("UserAgentManager"));
    load();
}

UserAgentManager::~UserAgentManager()
{
    save();

    if (m_addAgentDialog)
        delete m_addAgentDialog;
    if (m_userAgentsWindow)
        delete m_userAgentsWindow;
}

const QString &UserAgentManager::getUserAgentCategory() const
{
    return m_activeAgentCategory;
}

const UserAgent &UserAgentManager::getUserAgent() const
{
    return m_activeAgent;
}

void UserAgentManager::setActiveAgent(const QString &category, const UserAgent &agent)
{
    m_activeAgentCategory = category;
    m_activeAgent = agent;
    m_settings->setValue(BrowserSetting::CustomUserAgent, true);
    m_profile->setHttpUserAgent(m_activeAgent.Value);

    QTimer::singleShot(10, this, [this](){
        Q_EMIT updateUserAgents();
    });
}

void UserAgentManager::clearUserAgents()
{
    m_userAgents.clear();
}

void UserAgentManager::addUserAgents(const QString &category, std::vector<UserAgent> &&userAgents)
{
    m_userAgents[category] = std::move(userAgents);
}

void UserAgentManager::modifyWindowFinished()
{
    Q_EMIT updateUserAgents();
}

void UserAgentManager::disableActiveAgent()
{
    m_activeAgent = UserAgent();
    m_activeAgentCategory.clear();
    m_settings->setValue(BrowserSetting::CustomUserAgent, false);
    m_profile->setHttpUserAgent(QString());

    Q_EMIT updateUserAgents();
}

void UserAgentManager::addUserAgent()
{
    if (!m_addAgentDialog)
    {
        // Setup dialog to add a user agent
        m_addAgentDialog = new AddUserAgentDialog;

        // Add categories to dialog
        auto uaCategories = m_userAgents.keys();
        for (const auto &cat : uaCategories)
            m_addAgentDialog->addAgentCategory(cat);

        connect(m_addAgentDialog, &AddUserAgentDialog::userAgentAdded, this, &UserAgentManager::onUserAgentAdded);
    }
    m_addAgentDialog->show();
}

void UserAgentManager::modifyUserAgents()
{
    if (!m_userAgentsWindow)
        m_userAgentsWindow = new UserAgentsWindow(this);
    m_userAgentsWindow->loadUserAgents();
    m_userAgentsWindow->show();
}

void UserAgentManager::onUserAgentAdded()
{
    UserAgent newAgent;
    newAgent.Name = m_addAgentDialog->getAgentName();
    newAgent.Value = m_addAgentDialog->getAgentValue();

    QString category = m_addAgentDialog->getCategory();
    auto it = m_userAgents.find(category);
    if (it != m_userAgents.end())
        it->push_back(newAgent);
    else
    {
        std::vector<UserAgent> agentList;
        agentList.push_back(newAgent);
        m_userAgents.insert(category, std::move(agentList));
    }

    Q_EMIT updateUserAgents();
}

void UserAgentManager::load()
{
    QFile dataFile(m_settings->getPathValue(BrowserSetting::UserAgentsFile));
    if (!dataFile.exists() || !dataFile.open(QIODevice::ReadOnly))
        return;

    // Attempt to parse data file
    QByteArray uaData = dataFile.readAll();
    dataFile.close();

    QJsonDocument uaDocument(QJsonDocument::fromJson(uaData));
    if (!uaDocument.isObject())
        return;

    QJsonObject uaObj = uaDocument.object();

    /* load a format like the following
     * {
     *     "categoryName1": [
     *       { "Name": "UA Name 1", "Value": "UA Value 1" },
     *       { "Name": "UA Name 2", "Value": "UA Value 2", "Active": true }
     *     ],
     *     "categoryName2": [ ... ]
     * }
     */

    QString categoryName;
    for (auto it = uaObj.begin(); it != uaObj.end(); ++it)
    {
        auto itVal = it.value();
        if (!itVal.isArray())
            continue;

        categoryName = it.key();
        std::vector<UserAgent> agents;
        QJsonArray uaArray = itVal.toArray();
        for (auto agentIt = uaArray.begin(); agentIt != uaArray.end(); ++agentIt)
        {
            auto agentObj = agentIt->toObject();
            UserAgent currentAgent;
            currentAgent.Name = agentObj.value("Name").toString();
            currentAgent.Value = agentObj.value("Value").toString();

            // Check if user agent is toggled as active
            auto isActiveIt = agentObj.find("Active");
            if (isActiveIt != agentObj.end())
            {
                m_activeAgentCategory = categoryName;
                m_activeAgent = currentAgent;
            }

            agents.push_back(currentAgent);
        }

        m_userAgents.insert(categoryName, std::move(agents));
    }
}

void UserAgentManager::save()
{
    QFile dataFile(m_settings->getPathValue(BrowserSetting::UserAgentsFile));
    if (!dataFile.open(QIODevice::WriteOnly))
        return;

    // Construct a QJsonDocument containing the data in our UA map, and write to the data file
    QJsonObject uaObj;
    bool hasActiveAgent = !m_activeAgent.Name.isNull();
    for (auto it = m_userAgents.begin(); it != m_userAgents.end(); ++it)
    {
        bool activeAgentCategory = (hasActiveAgent && m_activeAgentCategory.compare(it.key()) == 0);
        QJsonArray agentArray;
        const std::vector<UserAgent> &agentList = it.value();
        for (auto uaIt = agentList.cbegin(); uaIt != agentList.cend(); ++uaIt)
        {
            QJsonObject agent;
            agent.insert("Name", uaIt->Name);
            agent.insert("Value", uaIt->Value);
            if (activeAgentCategory
                    && uaIt->Name.compare(m_activeAgent.Name) == 0
                    && uaIt->Value.compare(m_activeAgent.Value) == 0)
                agent.insert("Active", true);
            agentArray.append(QJsonValue(agent));
        }

        uaObj.insert(it.key(), QJsonValue(agentArray));
    }
    QJsonDocument uaDoc(uaObj);

    dataFile.write(uaDoc.toJson());
    dataFile.close();
}
