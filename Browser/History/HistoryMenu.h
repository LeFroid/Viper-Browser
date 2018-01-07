#ifndef HISTORYMENU_H
#define HISTORYMENU_H

#include <QIcon>
#include <QMenu>
#include <QUrl>

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

    /// Adds an item to the history menu, given a name, title and favicon
    void addHistoryItem(const QUrl &url, const QString &title, const QIcon &favicon);

    /// Clears the history entries from the menu
    void clearItems();

signals:
    /// Emitted when the user activates a history item on the menu, requesting a page load for the given url
    void loadUrl(const QUrl &url);

private slots:
    /// Resets the contents of the menu to the most recently visited items
    void resetItems();

private:
    /// Binds the pageVisited signal from the \ref HistoryManager to a slot that adds the
    /// page to the top of the history menu. Also binds the reset menu signal to the reset items slot
    void setup();

protected:
    /// "Show all history" menu action
    QAction *m_actionShowHistory;

    /// "Clear recent history" menu action
    QAction *m_actionClearHistory;
};

#endif // HISTORYMENU_H
