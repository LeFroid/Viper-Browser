#include "UserAgentMenu.h"
#include "BrowserApplication.h"
#include "Settings.h"
#include "UserAgentManager.h"

#include <memory>
#include <QActionGroup>

UserAgentMenu::UserAgentMenu(QWidget *parent) :
    QMenu(parent),
    m_userAgentGroup(new QActionGroup(this))
{
    setup();
}

UserAgentMenu::UserAgentMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent),
    m_userAgentGroup(new QActionGroup(this))
{
    setup();
}

UserAgentMenu::~UserAgentMenu()
{
}

void UserAgentMenu::resetItems()
{
    std::shared_ptr<Settings> settings = sBrowserApplication->getSettings();

    // First remove old items, then repopulate the menu
    clear();
    UserAgentManager *uaManager = sBrowserApplication->getUserAgentManager();

    QAction *currItem = addAction(tr("Default"), uaManager, &UserAgentManager::disableActiveAgent);
    currItem->setCheckable(true);
    m_userAgentGroup->addAction(currItem);
    currItem->setChecked(true);

    QMenu *subMenu = nullptr;
    for (auto it = uaManager->getAgentIterBegin(); it != uaManager->getAgentIterEnd(); ++it)
    {
        subMenu = new QMenu(it.key(), this);
        addMenu(subMenu);

        bool hasActiveAgentInMenu = (settings->getValue(BrowserSetting::CustomUserAgent).toBool()
                && (uaManager->getUserAgentCategory().compare(it.key()) == 0));

        for (auto agentIt = it->cbegin(); agentIt != it->cend(); ++agentIt)
        {
            QAction *action = subMenu->addAction(agentIt->Name);
            action->setData(agentIt->Value);
            action->setCheckable(true);
            m_userAgentGroup->addAction(action);
            if (hasActiveAgentInMenu && (agentIt->Name.compare(uaManager->getUserAgent().Name) == 0))
                action->setChecked(true);
        }

        connect(subMenu, &QMenu::triggered, [=](QAction *action){
            UserAgent agent;
            agent.Name = action->text();
            // Remove first character from UA name, as it begins with a '&'
            agent.Name = agent.Name.right(agent.Name.size() - 1);
            agent.Value = action->data().toString();
            uaManager->setActiveAgent(it.key(), agent);
            action->setChecked(true);
        });
    }

    addSeparator();
    addAction(tr("Add User Agent"), uaManager, &UserAgentManager::addUserAgent);
    addAction(tr("Modify User Agents"), uaManager, &UserAgentManager::modifyUserAgents);
}

void UserAgentMenu::setup()
{
    connect(sBrowserApplication->getUserAgentManager(), &UserAgentManager::updateUserAgents, this, &UserAgentMenu::resetItems);
}
