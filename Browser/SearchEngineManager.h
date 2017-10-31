#ifndef SEARCHENGINEMANAGER_H
#define SEARCHENGINEMANAGER_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

/**
 * @class SearchEngineManager
 * @brief Handles search engine data used by the \ref SearchEngineLineEdit widget
 */
class SearchEngineManager : public QObject
{
    Q_OBJECT
public:
    /// Constructs the search engine manager
    explicit SearchEngineManager(QObject *parent = nullptr);

    /// Search engine manager destructor, will save search engines if they have been modified
    virtual ~SearchEngineManager();

    /// Returns the search engine manager singleton
    static SearchEngineManager &instance();

    /// Returns the name of the default search engine
    const QString &getDefaultSearchEngine() const;

    /// Sets the default search engine to the given name (identifier) of a search engine in the collection.
    void setDefaultSearchEngine(const QString &name);

    /// Returns a list of the names of all search engines stored by the search engine manager
    QList<QString> getSearchEngineNames() const;

    /// Returns the query string associated with the given search engine, or a null string if not found
    QString getQueryString(const QString &searchEngine) const;

    /// Loads search engine information from the given file, including information such as the default engine
    void loadSearchEngines(const QString &fileName);

signals:
    /// Emitted when a new search engine has been added to the collection
    void engineAdded(const QString &name);

    /// Emitted when a search engine has been removed from the collection
    void engineRemoved(const QString &name);

public slots:
    /// Adds a search engine to the collection, with the given reference name and a search URL
    void addSearchEngine(const QString &name, const QString &query);

    /**
     * @brief removeSearchEngine Removes the given search engine from the collection
     * @param name Name of the search engine
     *
     * Removes the given search engine from the collection. If the search engine is currently the
     * default, the first search engine in the hashmap will be set to the new default.
     */
    void removeSearchEngine(const QString &name);

private:
    /// Saves search engine data to disk
    void save();

private:
    /// Flag that determines whether or not the search engine data has been modified (which requires a save to disk at program exit)
    bool m_shouldSave;

    /// Path of the search engine file
    QString m_searchEngineFile;

    /// Name of the default search engine
    QString m_defaultEngine;

    /// Hashmap of search engines, where key = user friendly name of the engine, and value = the search URL
    QHash<QString, QString> m_searchEngines;
};

#endif // SEARCHENGINEMANAGER_H
