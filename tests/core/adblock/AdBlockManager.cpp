#include "AdBlockManager.h"
#include "AdBlockLog.h"
#include "AdBlockRequestHandler.h"
#include "Bitfield.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <string>

#include <QDebug>

namespace adblock
{

class AdBlockModel
{
public:
    AdBlockModel() {}
};

AdBlockManager::AdBlockManager(const ViperServiceLocator &, QObject *parent) :
    QObject(parent),
    m_filterContainer(),
    m_downloadManager(nullptr),
    m_enabled(false),
    m_configFile("AdBlockStub.json"),
    m_subscriptionDir(),
    m_cosmeticJSTemplate(),
    m_subscriptions(),
    m_resourceMap(),
    m_resourceContentTypeMap(),
    m_domainStylesheetCache(24),
    m_jsInjectionCache(24),
    m_emptyStr(),
    m_adBlockModel(nullptr),
    m_log(nullptr),
    m_requestHandler(nullptr)
{
}

AdBlockManager::~AdBlockManager()
{
}

void AdBlockManager::onSettingChanged(BrowserSetting, const QVariant &)
{
}

void AdBlockManager::setEnabled(bool value)
{
    m_enabled = value;

    clearFilters();
    if (value)
        extractFilters();
}

void AdBlockManager::updateSubscriptions()
{
    if (!m_enabled)
        return;
}

void AdBlockManager::installResource(const QUrl &url)
{
    if (!url.isValid())
        return;
}

void AdBlockManager::installSubscription(const QUrl &url)
{
    if (!url.isValid())
        return;
}

void AdBlockManager::createUserSubscription()
{
}

void AdBlockManager::loadStarted(const QUrl &/*url*/)
{
}

AdBlockModel *AdBlockManager::getModel()
{
    return nullptr;
}

const QString &AdBlockManager::getStylesheet(const URL &/*url*/) const
{
    return m_emptyStr;
}

const QString &AdBlockManager::getDomainStylesheet(const URL &/*url*/)
{
    return m_emptyStr;
}

const QString &AdBlockManager::getDomainJavaScript(const URL &/*url*/)
{
    return m_emptyStr;
}

bool AdBlockManager::shouldBlockRequest(QWebEngineUrlRequestInfo &/*info*/, const QUrl &/*firstPartyUrl*/)
{
    return false;
}

quint64 AdBlockManager::getRequestsBlockedCount() const
{
    return 0;
}

int AdBlockManager::getNumberAdsBlocked(const QUrl &/*url*/) const
{
    return 0;
}

QString AdBlockManager::getResource(const QString &key) const
{
    return m_resourceMap.value(key);
}

QString AdBlockManager::getResourceContentType(const QString &key) const
{
    return m_resourceContentTypeMap.value(key);
}

int AdBlockManager::getNumSubscriptions() const
{
    return static_cast<int>(m_subscriptions.size());
}

const Subscription *AdBlockManager::getSubscription(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return nullptr;

    return &m_subscriptions.at(index);
}

void AdBlockManager::toggleSubscriptionEnabled(int index)
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return;

    Subscription &sub = m_subscriptions.at(index);
    sub.setEnabled(!sub.isEnabled());

    // Reset filter data
    reloadSubscriptions();
}

void AdBlockManager::removeSubscription(int index)
{
    if (index < 0 || index >= static_cast<int>(m_subscriptions.size()))
        return;

    // Point iterator to subscription
    auto it = m_subscriptions.cbegin() + index;

    // Delete the subscription file before removal
    QFile subFile(it->getFilePath());
    if (subFile.exists())
    {
        if (!subFile.remove())
            qDebug() << "[Advertisement Blocker]: Could not remove subscription file " << subFile.fileName();
    }

    m_subscriptions.erase(it);

    reloadSubscriptions();
}

void AdBlockManager::reloadSubscriptions()
{
    clearFilters();
    extractFilters();
}

void AdBlockManager::loadDynamicTemplate()
{
    QFile templateFile(QLatin1String(":/AdBlock.js"));
    if (!templateFile.open(QIODevice::ReadOnly))
        return;

    m_cosmeticJSTemplate = templateFile.readAll();
    templateFile.close();
}

void AdBlockManager::loadUBOResources()
{
    QDir resourceDir(QString("%1%2%3").arg(m_subscriptionDir).arg(QDir::separator()).arg(QLatin1String("resources")));
    if (!resourceDir.exists())
        resourceDir.mkpath(QStringLiteral("."));

    // Iterate through files in directory, loading into m_resourceMap
    QDirIterator resourceItr(resourceDir.absolutePath(), QDir::Files);
    while (resourceItr.hasNext())
    {
        loadResourceFile(resourceItr.next());
    }
}

void AdBlockManager::loadResourceAliases()
{
}

void AdBlockManager::loadResourceFile(const QString &path)
{
    QFile f(path);
    if (!f.exists() || !f.open(QIODevice::ReadOnly))
        return;

    bool readingValue = false;
    QString currentKey, mimeType;
    QByteArray currentValue;
    QList<QByteArray> contents = f.readAll().split('\n');
    f.close();
    for (int i = 0; i < contents.size(); ++i)
    {
        const QByteArray &line = contents.at(i);
        if ((!readingValue && line.isEmpty()) || line.startsWith('#'))
            continue;

        // Extract key from line if not loading a value associated with a key
        if (!readingValue)
        {
            int sepIdx = line.indexOf(' ');
            if (sepIdx < 0)
                currentKey = line;
            else
            {
                currentKey = line.left(sepIdx);
                mimeType = line.mid(sepIdx + 1);
                m_resourceContentTypeMap.insert(currentKey, mimeType);
            }
            readingValue = true;
        }
        else
        {
            // Append currentValue with line if not empty.
            if (!line.isEmpty())
            {
                currentValue.append(line);
                if (mimeType.contains(QLatin1String("javascript")))
                    currentValue.append('\n');
            }
            else
            {
                // Insert key-value pair into map once an empty line is reached and search for next key
                m_resourceMap.insert(currentKey, QString(currentValue));
                currentValue.clear();
                readingValue = false;
            }
        }
    }
}

void AdBlockManager::loadSubscriptions()
{
    if (!m_enabled)
        return;

    QFile configFile(m_configFile);
    if (!configFile.exists() || !configFile.open(QIODevice::ReadOnly))
        return;

    // Attempt to parse config/subscription info file
    QByteArray configData = configFile.readAll();
    configFile.close();

    // Add subscriptions to container
    QJsonDocument configDoc(QJsonDocument::fromJson(configData));
    QJsonObject configObj = configDoc.object();
    QJsonObject subscriptionObj;
    for (auto it = configObj.begin(); it != configObj.end(); ++it)
    {
        const QString key = it.key();
        if (key.compare(QLatin1String("requests_blocked")) == 0)
        {
            continue;
        }
        Subscription subscription(key);

        subscriptionObj = it.value().toObject();
        subscription.setEnabled(subscriptionObj.value(QLatin1String("enabled")).toBool());

        // Get last update as unix epoch value
        bool ok;
        qint64 lastUpdateUInt = subscriptionObj.value(QLatin1String("last_update")).toVariant().toLongLong(&ok);
        QDateTime lastUpdate = (ok && lastUpdateUInt > 0 ? QDateTime::fromSecsSinceEpoch(lastUpdateUInt) : QDateTime::currentDateTime());
        subscription.setLastUpdate(lastUpdate);

        // Attempt to get next update time as unix epoch value
        qint64 nextUpdateUInt = subscriptionObj.value(QLatin1String("next_update")).toVariant().toLongLong(&ok);
        if (ok && nextUpdateUInt > 0)
        {
            QDateTime nextUpdate = QDateTime::fromSecsSinceEpoch(nextUpdateUInt);
            subscription.setNextUpdate(nextUpdate);
        }

        // Get source URL of the subscription
        QString source = subscriptionObj.value(QLatin1String("source")).toString();
        if (!source.isEmpty())
            subscription.setSourceUrl(QUrl(source));

        m_subscriptions.push_back(std::move(subscription));
    }

    extractFilters();
}

void AdBlockManager::clearFilters()
{
    m_filterContainer.clearFilters();
}

void AdBlockManager::extractFilters()
{
    for (Subscription &s : m_subscriptions)
    {
        // calling load() does nothing if subscription is disabled
        s.load(this);
    }
    m_filterContainer.extractFilters(m_subscriptions);
}

void AdBlockManager::save()
{
}

}

#include "moc_AdBlockManager.cpp"
