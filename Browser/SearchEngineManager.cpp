#include "SearchEngineManager.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

SearchEngineManager::SearchEngineManager(QObject *parent) :
    QObject(parent),
    m_shouldSave(false),
    m_searchEngineFile(),
    m_defaultEngine(),
    m_searchEngines()
{
}

SearchEngineManager::~SearchEngineManager()
{
    if (m_shouldSave)
        save();
}

SearchEngineManager &SearchEngineManager::instance()
{
    static SearchEngineManager manager;
    return manager;
}

const QString &SearchEngineManager::getDefaultSearchEngine() const
{
    return m_defaultEngine;
}

void SearchEngineManager::setDefaultSearchEngine(const QString &name)
{
    // Ignore if already the default search engine
    if (m_defaultEngine.compare(name) == 0)
        return;

    // Ignore if search engine not found
    if (!m_searchEngines.contains(name))
        return;

    m_defaultEngine = name;
    m_shouldSave = true;
    emit defaultEngineChanged(m_defaultEngine);
}

QList<QString> SearchEngineManager::getSearchEngineNames() const
{
    return m_searchEngines.keys();
}

QString SearchEngineManager::getQueryString(const QString &searchEngine) const
{
    auto it = m_searchEngines.find(searchEngine);
    if (it != m_searchEngines.end())
        return it.value();

    return QString();
}

void SearchEngineManager::addSearchEngine(const QString &name, const QString &query)
{
    if (name.isEmpty() || query.isEmpty())
        return;

    // Check if already in collection, if so, then simply update query string associated with name
    if (m_searchEngines.contains(name))
    {
        m_searchEngines[name] = query;
        m_shouldSave = true;
        return;
    }

    m_searchEngines.insert(name, query);
    m_shouldSave = true;
    emit engineAdded(name);
}

void SearchEngineManager::removeSearchEngine(const QString &name)
{
    if (!m_searchEngines.contains(name))
        return;

    m_searchEngines.remove(name);
    m_shouldSave = true;

    // Check if we need to assign a new default search engine (will set to first item in hashmap)
    if (m_defaultEngine.compare(name) == 0)
        m_defaultEngine = m_searchEngines.begin().key();

    emit engineRemoved(name);
}

void SearchEngineManager::loadSearchEngines(const QString &fileName)
{
    QFile dataFile(fileName);

    // Load from template if provided file does not exist
    if (!dataFile.exists())
        dataFile.setFileName(QStringLiteral(":/search_engines.json"));

    if (!dataFile.open(QIODevice::ReadOnly))
        return;

    m_searchEngineFile = fileName;

    // Attempt to parse data file
    QByteArray engineData = dataFile.readAll();
    dataFile.close();

    QJsonDocument engineDoc(QJsonDocument::fromJson(engineData));
    if (!engineDoc.isObject())
        return;

    QJsonObject engineObj = engineDoc.object();
    /*
     * Load data from root object, which has following format:
     * {
     *     "Default": "Name1",
     *     "Engines": [
     *       {
     *           "Name": "Name1",
     *           "Query": "https://..."
     *       }, { ... }, { ... }
     *     ]
     * }
     */
    QJsonArray engineArray = engineObj.value("Engines").toArray();
    QJsonObject currentEngine;
    for (auto it = engineArray.begin(); it != engineArray.end(); ++it)
    {
        currentEngine = it->toObject();
        m_searchEngines.insert(currentEngine.value("Name").toString(), currentEngine.value("Query").toString());
    }

    // Set search engine to the default value
    m_defaultEngine = engineObj.value("Default").toString();
}

void SearchEngineManager::save()
{
    if (m_searchEngineFile.isNull() || m_searchEngineFile.isEmpty())
        return;

    QFile saveFile(m_searchEngineFile);
    if (!saveFile.open(QIODevice::WriteOnly))
        return;

    // Construct the root JSON object and set default engine
    QJsonObject searchObj;
    searchObj.insert("Default", m_defaultEngine);

    // Create array of search engines and add to root object
    QJsonArray engineArray;
    for (auto it = m_searchEngines.begin(); it != m_searchEngines.end(); ++it)
    {
        QJsonObject currentEngine;
        currentEngine.insert("Name", it.key());
        currentEngine.insert("Query", it.value());

        engineArray.append(QJsonValue(currentEngine));
    }

    searchObj.insert("Engines", QJsonValue(engineArray));

    // Format root object as a document, and write to the file
    QJsonDocument searchDoc(searchObj);
    saveFile.write(searchDoc.toJson());
    saveFile.close();
}
