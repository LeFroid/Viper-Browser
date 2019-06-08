#include "UserAgentMenu.h"
#include "BrowserApplication.h"
#include "Settings.h"
#include "UserAgentManager.h"

#include <memory>
#include <QActionGroup>

UserAgentMenu::UserAgentMenu(QWidget *parent) :
    QMenu(parent),
    m_settings(nullptr),
    m_userAgentManager(nullptr),
    m_userAgentGroup(new QActionGroup(this))
{
}

UserAgentMenu::UserAgentMenu(const QString &title, QWidget *parent) :
    QMenu(title, parent),
    m_settings(nullptr),
    m_userAgentManager(nullptr),
    m_userAgentGroup(new QActionGroup(this))
{
}

UserAgentMenu::~UserAgentMenu()
{
}

void UserAgentMenu::setServiceLocator(const ViperServiceLocator &serviceLocator)
{
    if (m_settings != nullptr || m_userAgentManager != nullptr)
        return;

    m_settings = serviceLocator.getServiceAs<Settings>("Settings");
    m_userAgentManager = serviceLocator.getServiceAs<UserAgentManager>("UserAgentManager");

    if (m_userAgentManager != nullptr)
    {
        setup();
        resetItems();
    }
}

void UserAgentMenu::resetItems()
{
    if (!m_settings || !m_userAgentManager)
        return;

    // First remove old items, then repopulate the menu
    clear();

    QAction *currItem = addAction(tr("Default"), m_userAgentManager, &UserAgentManager::disableActiveAgent);
    currItem->setCheckable(true);
    m_userAgentGroup->addAction(currItem);
    currItem->setChecked(true);

    QMenu *subMenu = nullptr;

    const UserAgent &activeUA = m_userAgentManager->getUserAgent();

    for (auto it = m_userAgentManager->begin(); it != m_userAgentManager->end(); ++it)
    {
        subMenu = new QMenu(it.key(), this);
        addMenu(subMenu);

        bool hasActiveAgentInMenu = (m_settings->getValue(BrowserSetting::CustomUserAgent).toBool()
                && (m_userAgentManager->getUserAgentCategory().compare(QString(it.key())) == 0));

        for (const UserAgent &agent : *it)
        {
            QAction *action = subMenu->addAction(agent.Name);
            action->setData(agent.Value);
            action->setCheckable(true);

            m_userAgentGroup->addAction(action);

            if (hasActiveAgentInMenu && (agent.Name.compare(activeUA.Name) == 0))
                action->setChecked(true);
        }

        connect(subMenu, &QMenu::triggered, subMenu, [=](QAction *action){
            UserAgent agent;
            agent.Name = action->text();

            // Remove first character from UA name, if it begins with a '&'
            if (agent.Name.at(0) == QChar('&'))
                agent.Name = agent.Name.right(agent.Name.size() - 1);

            agent.Value = action->data().toString();
            m_userAgentManager->setActiveAgent(it.key(), agent);
            action->setChecked(true);
        });
    }

    addSeparator();
    addAction(tr("Add User Agent"),     m_userAgentManager, &UserAgentManager::addUserAgent);
    addAction(tr("Modify User Agents"), m_userAgentManager, &UserAgentManager::modifyUserAgents);
}

void UserAgentMenu::setup()
{
    connect(m_userAgentManager, &UserAgentManager::updateUserAgents, this, &UserAgentMenu::resetItems);
}
