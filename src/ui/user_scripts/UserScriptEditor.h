#ifndef USERSCRIPTEDITOR_H
#define USERSCRIPTEDITOR_H

#include <QMainWindow>

namespace Ui {
    class UserScriptEditor;
}

class JavaScriptHighlighter;

/**
 * @class UserScriptEditor
 * @brief Allows the user to modify user scripts with a code editor widget
 */
class UserScriptEditor : public QMainWindow
{
    Q_OBJECT

public:
    /// Constructs the user script editor
    explicit UserScriptEditor(QWidget *parent = nullptr);

    /// UserScriptEditor destructor
    ~UserScriptEditor();

    /**
     * @brief Sets all of the data fields that are necessary in order to modify a user script
     * @param name Name of the user script
     * @param code Code section of the script
     * @param filePath Full path to the user script file
     * @param rowIndex Row of the index of the script in the \ref UserScriptModel
     */
    void setScriptInfo(const QString &name, const QString &code, const QString &filePath, int rowIndex);

Q_SIGNALS:
    /// Emitted when the user script has been modified and the editor has closed. Will result in the script being reloaded
    void scriptModified(int rowIndex);

private Q_SLOTS:
    /// Toggles the visibility and focus of the find text widget
    void toggleFindTextWidget();

    /// Saves the script to file, along with any changes that were made
    void saveScript();

    /// Handles script content modification events
    void onScriptModified(bool changed);

    /// Called when the text finder widget aesthetically modifies the document, set so the onScriptModified slot
    /// can ignore the change
    void onTextFindPseudoModify();

protected:
    /// Handles close events. If the script has been modified without being saved, a dialog will appear
    /// asking if the user wishes to save their changes
    void closeEvent(QCloseEvent *event) override;

private:
    /// UI elements
    Ui::UserScriptEditor *ui;

    /// JavaScript syntax highlighter for the user script contents
    JavaScriptHighlighter *m_highlighter;

    /// Stores the path of the file being edited
    QString m_filePath;

    /// Index of the script in the \ref UserScriptModel
    int m_rowIndex;

    /// True if the script has been modified without being saved, false if else
    bool m_scriptModified;

    /// Flag that is used to ignore "changes" to the document made by the text finding widget
    bool m_ignoreFindModify;
};

#endif // USERSCRIPTEDITOR_H
