#ifndef SEARCHTAB_H
#define SEARCHTAB_H

#include <QWidget>

namespace Ui {
    class SearchTab;
}

class AddSearchEngineDialog;

/**
 * @class SearchTab
 * @brief Allows the user to modify the search engines used in the \ref SearchEngineLineEdit widget
 */
class SearchTab : public QWidget
{
    Q_OBJECT
public:
    explicit SearchTab(QWidget *parent = 0);
    ~SearchTab();

private Q_SLOTS:
    /// Called when the default search engine has changed to the item in the combo box at the given index
    void onDefaultEngineChanged(int index);

    /// Called when the user clicks the "Remove" button after selecting a search engine to be removed
    void onRemoveSelectedEngine();

    /// Displays the dialog to add a search engine
    void showAddEngineDialog();

    /// Adds the given search engine to the collection and to the local UI elements
    void addSearchEngine(const QString &name, const QString &queryUrl);

private:
    /// Loads search engine data from the \ref SearchEngineManager
    void loadSearchEngines();

private:
    /// Search tab user interface elements
    Ui::SearchTab *ui;

    /// Dialog used to add search engines to the collection
    AddSearchEngineDialog *m_addEngineDialog;
};

#endif // SEARCHTAB_H
