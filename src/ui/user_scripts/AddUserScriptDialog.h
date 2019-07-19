#ifndef ADDUSERSCRIPTDIALOG_H
#define ADDUSERSCRIPTDIALOG_H

#include <QDialog>

namespace Ui {
    class AddUserScriptDialog;
}

/**
 * @class AddUserScriptDialog
 * @brief Used prior to the creation of a new user script, allows the user to enter some
 *        meta-information such as the script's name, namespace, description and version
 */
class AddUserScriptDialog : public QDialog
{
    Q_OBJECT

public:
    /// Constructs the dialog with an optional pointer to a parent widget
    explicit AddUserScriptDialog(QWidget *parent = nullptr);

    /// AddUserScriptDialolg destructor
    ~AddUserScriptDialog();

Q_SIGNALS:
    /// Emitted if the user hits the "Ok" button, confirming the creation of a new user script
    void informationEntered(const QString &name, const QString &nameSpace, const QString &description, const QString &version);

private Q_SLOTS:
    /// Called when the "Ok" button is entered, emits the informationEntered(..) signal with script meta-information
    void onAccepted();

private:
    /// Pointer to the UI form elements
    Ui::AddUserScriptDialog *ui;
};

#endif // ADDUSERSCRIPTDIALOG_H
