#ifndef SEARCHENGINELINEEDIT_H
#define SEARCHENGINELINEEDIT_H

#include "SearchEngineManager.h"

#include <QHash>
#include <QLineEdit>
#include <QUrl>

class FaviconManager;
class HttpRequest;
class QMenu;
class QToolButton;

/**
 * @class SearchEngineLineEdit
 * @brief A line edit widget that is incorporated in the browser tool bar for
 *        the purpose of quickly accessing the user's preferred search engine
 *        for a search term.
 */
class SearchEngineLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    /// Constructs the search engine line edit with a given parent widget
    explicit SearchEngineLineEdit(QWidget *parent = nullptr);

    /// Sets the pointer to the favicon manager
    void setFaviconManager(FaviconManager *faviconManager);

Q_SIGNALS:
    /// Emitted when the user enters a search term and the current browser tab should load the given request
    void requestPageLoad(const HttpRequest &request);

private Q_SLOTS:
    /// Sets the current search engine to the engine that is mapped to the given name
    void setSearchEngine(const QString &name);

    /// Appends the given search engine to the menu
    void addSearchEngine(const QString &name);

    /**
     * @brief removeSearchEngine Removes the given search engine from the menu.
     * @param name Name of the search engine
     *
     * Will remove the given search engine from the dropdown menu, and if the search
     * engine was in use at the time of removal, the search engine will be changed
     * to the default.
     */
    void removeSearchEngine(const QString &name);

protected:
    /// Paints the line edit with the search icon shown on its leftmost end
    virtual void resizeEvent(QResizeEvent *event) override;

private:
    /// Loads search engines, stored by the \ref SearchEngineManager, into the line edit
    void loadSearchEngines();

private:
    /// Favicon manager
    FaviconManager *m_faviconManager;

    /// Button indicating that the line edit is used for querying a selected search engine
    QToolButton *m_searchButton;

    /// Menu that is attached to the search icon button, which allows the user to select a search engine
    QMenu *m_searchEngineMenu;

    /// Name of the current search engine
    QString m_currentEngineName;

    /// Query string of the current search engine
    QString m_currentEngineQuery;

    /// Current search engine
    SearchEngine m_currentEngine;
};

#endif // SEARCHENGINELINEEDIT_H
