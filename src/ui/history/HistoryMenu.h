#ifndef HISTORYMENU_H
#define HISTORYMENU_H

#include "ServiceLocator.h"

#include <QIcon>
#include <QMenu>
#include <QUrl>

class FaviconManager;
class HistoryManager;

/**
 * @class HistoryMenu
 * @brief Manages the browser's history menu, contained in the \ref MainWindow menu bar
 */
class HistoryMenu : public QMenu
{
    friend class MainWindow;

    Q_OBJECT

public:
    /// Constructs the history menu with the given parent
    HistoryMenu(QWidget *parent = nullptr);

    /// Constructs the history menu with a title and a parent.
    HistoryMenu(const QString &title, QWidget *parent = nullptr);

    /// Destroys the menu
    virtual ~HistoryMenu();

    /// Passes a reference to the service locator, which allows the history menu
    /// to gather its dependencies on the \ref HistoryManager and \ref FaviconManager
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

    /// Adds an item to the history menu, given a name, title and favicon
    void addHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon);

    /// Adds an item to the top of the history menu, given a name, title and favicon
    void prependHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon);

    /// Clears the history entries from the menu
    void clearItems();

Q_SIGNALS:
    /// Emitted when the user activates a history item on the menu, requesting a page load for the given url
    void loadUrl(const QUrl &url);

private Q_SLOTS:
    /// Resets the contents of the menu to the most recently visited items
    void resetItems();

    /// Called when a page has been visited by the user
    void onPageVisited(const QUrl &url, const QString &title);

private:
    /// Binds the pageVisited signal from the \ref HistoryManager to a slot that adds the
    /// page to the top of the history menu. Also binds the reset menu signal to the reset items slot
    void setup();

    /// Clears any entries at the bottom of the history menu, if
    void clearOldestEntries();

protected:
    /// History manager
    HistoryManager *m_historyManager;

    /// Favicon manager
    FaviconManager *m_faviconManager;

    /// "Show all history" menu action
    QAction *m_actionShowHistory;

    /// "Clear recent history" menu action
    QAction *m_actionClearHistory;
};

#endif // HISTORYMENU_H
