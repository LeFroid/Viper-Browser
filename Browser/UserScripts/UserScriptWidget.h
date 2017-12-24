#ifndef USERSCRIPTWIDGET_H
#define USERSCRIPTWIDGET_H

#include <QWidget>

namespace Ui {
    class UserScriptWidget;
}

/**
 * @class UserScriptWidget
 * @brief Acts as a management window for user scripts. The user can install remote scripts by URL,
 *        create new scripts locally, modify existing scripts, delete scripts from storage, and enable
 *        or disable existing scripts.
 */
class UserScriptWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the management window. No parent widget should be given
    explicit UserScriptWidget(QWidget *parent = nullptr);

    /// Destroys the UI elements associated with the object
    ~UserScriptWidget();

protected:
    /// Called to adjust the proportions of the columns belonging to the table views
    virtual void resizeEvent(QResizeEvent *event) override;

private slots:
    /// Called when an item in the table view is clicked. This enables the edit and delete buttons
    void onItemClicked(const QModelIndex &index);

    /// Called when the delete button is clicked. Will prompt the user for confirmation before deleting a user script
    void onDeleteButtonClicked();

    /// Called when the edit button is clicked. Will allow the user to edit the contents of the selected user script
    void onEditButtonClicked();

    /// Called when the install button is clicked. Spawns an input dialog and attempts to download and install the resource as a user script
    void onInstallButtonClicked();

private:
    /// Pointer to the user interface elements
    Ui::UserScriptWidget *ui;
};

#endif // USERSCRIPTWIDGET_H
