#ifndef CUSTOMFILTEREDITOR_H
#define CUSTOMFILTEREDITOR_H

#include <QMainWindow>

namespace Ui {
    class CustomFilterEditor;
}

/**
 * @class CustomFilterEditor
 * @ingroup AdBlock
 * @brief Allows the user to modify their own set of AdBlock Plus or uBlock Origin -styled
 *        web content filters
 */
class CustomFilterEditor : public QMainWindow
{
    Q_OBJECT

public:
    /// Constructs the ad block filter editor
    explicit CustomFilterEditor(QWidget *parent = nullptr);

    /// CustomFilterEditor destructor
    ~CustomFilterEditor();

signals:
    /// Emitted when the user-specified filters have been modified and the editor is closed or is closing.
    /// Will result in a reload of the user's blocking filters
    void filtersModified();

    /// Emitted if the user does not have any custom filters at the time of the editor loading, will result
    /// in the \ref AdBlockManager registering a new subscription for user-set filter rules
    void createUserSubscription();

private slots:
    /// Toggles the visibility and focus of the find text widget
    void toggleFindTextWidget();

    /// Saves the custom filters to file
    void saveFilters();

    /// Handles filter content modification events
    void onFiltersModified(bool changed);

    /// Called when the text finder widget aesthetically modifies the document, set so the onFiltersModified slot
    /// can ignore the change
    void onTextFindPseudoModify();

protected:
    /// Handles close events. If the filters have been modified without being saved, a dialog will appear
    /// asking if the user wishes to save their changes
    void closeEvent(QCloseEvent *event) override;

private:
    /// Loads the user blocking filters into the editor
    void loadUserFilters();

private:
    /// UI elements
    Ui::CustomFilterEditor *ui;

    /// Stores the path of the file being edited
    QString m_filePath;

    /// True if the filters have been modified without being saved, false if else
    bool m_filtersModified;

    /// Flag that is used to ignore "changes" to the document made by the text finding widget
    bool m_ignoreFindModify;
};

#endif // CUSTOMFILTEREDITOR_H
