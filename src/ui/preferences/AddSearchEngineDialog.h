#ifndef ADDSEARCHENGINEDIALOG_H
#define ADDSEARCHENGINEDIALOG_H

#include <QDialog>

namespace Ui {
    class AddSearchEngineDialog;
}

/**
 * @class AddSearchEngineDialog
 * @brief Input dialog that allows the user to add a new search engine to the
 *        collection used by the quick search tool.
 */
class AddSearchEngineDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddSearchEngineDialog(QWidget *parent = 0);
    ~AddSearchEngineDialog();

    /// Clears the values in the line edit widgets
    void clearValues();

    /// Returns the name assigned to the search engine
    QString getName() const;

    /// Returns the query URL associated with the search engine
    QString getQueryURL() const;

Q_SIGNALS:
    /// Emitted when the user hits the "Ok" button to confirm the addition of a new search engine
    void searchEngineAdded(const QString &name, const QString &queryUrl);

private:
    Ui::AddSearchEngineDialog *ui;
};

#endif // ADDSEARCHENGINEDIALOG_H
