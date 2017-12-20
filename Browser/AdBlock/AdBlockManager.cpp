#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "Settings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace AdBlock
{
    AdBlockManager::AdBlockManager() :
        m_enabled(true),
        m_subscriptionDir()
    {
        m_enabled = sBrowserApplication->getSettings()->getValue("AdBlockPlusEnabled").toBool();
        loadSubscriptions();
    }

    AdBlockManager &AdBlockManager::instance()
    {
        static AdBlockManager adBlockInstance;
        return adBlockInstance;
    }

    void AdBlockManager::loadSubscriptions()
    {
        if (!m_enabled)
            return;

        std::shared_ptr<Settings> settings = sBrowserApplication->getSettings();

        m_subscriptionDir = settings->getPathValue("AdBlockPlusDataDir");
        QDir subscriptionDir(m_subscriptionDir);
        if (!subscriptionDir.exists())
            subscriptionDir.mkpath(m_subscriptionDir);

        /*
        QFile configFile(settings->getPathValue("AdBlockPlusConfig"));
        if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
            return;

        // Attempt to parse config/subscription info file
        QByteArray configData = configFile.readAll();
        configFile.close();

        QJsonDocument configDoc(QJsonDocument::fromJson(configData));
        QJsonObject configObj = configDoc.object();
        for (auto it = configObj.begin(); it != configObj.end(); ++it)
        {
            QString subscriptionFile = it.key();
            bool enabled = it.value().toBool(false);
            if (enabled)
            {
                //AdBlockSubscription subscription(subscriptionFile);
            }
        }*/
    }
}
