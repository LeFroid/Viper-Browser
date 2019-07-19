#ifndef FILECHOOSERLINEEDIT_H
#define FILECHOOSERLINEEDIT_H

#include <QFileDialog>
#include <QLineEdit>

class QToolButton;

/**
 * @class FileChooserLineEdit
 * @brief Implementation of a line edit widget that includes
 *        a "Browse" button on its right side, that allows the
 *        user to select a file or directory to be displayed in
 *        the line edit section of the widget
 */
class FileChooserLineEdit : public QLineEdit
{
public:
    /// Constructs the line edit with a given parent widget
    explicit FileChooserLineEdit(QWidget *parent = nullptr);

    /// Sets the mode of the dialog associated with the browse button
    void setFileMode(QFileDialog::FileMode mode);

private Q_SLOTS:
    /// Called when the "Browse" button is clicked, spawns a file/folder chooser dialog
    void onClickBrowse();

protected:
    /// Paints the line edit with a button to the right that the user may click on to spawn a
    /// \ref QFileDialog
    virtual void resizeEvent(QResizeEvent *event) override;

protected:
    /// The "Browse..." button used to select a file or folder
    QToolButton *m_buttonBrowse;

    /// Mode of operation of the file chooser dialog
    QFileDialog::FileMode m_fileMode;
};

#endif // FILECHOOSERLINEEDIT_H
