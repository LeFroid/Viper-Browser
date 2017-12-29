#ifndef ADDBOOKMARKDIALOG_H
#define ADDBOOKMARKDIALOG_H

#include "BookmarkManager.h"
#include <QDialog>
#include <QString>

namespace Ui {
    class AddBookmarkDialog;
}

/**
 * @class AddBookmarkDialog
 * @brief Shown when the user adds a new bookmark to their collection through a menu
 *        option in the main window. Allows the user to change the bookmark name, its
 *        parent folder, and the may also remove the bookmark
 */
class AddBookmarkDialog : public QDialog
{
    Q_OBJECT

public:
    /// Constructs the dialog given a pointer to the bookmark manager and optionally a parent widget
    explicit AddBookmarkDialog(BookmarkManager *bookmarkMgr, QWidget *parent = 0);

    /// Dialog destructor
    ~AddBookmarkDialog();

    /// Sets the information fields of the newly added bookmark that will be displayed in the dialog
    void setBookmarkInfo(const QString &bookmarkName, const QString &bookmarkUrl);

signals:
    /// Emitted when browser windows should update their bookmark menu
    void updateBookmarkMenu();

private slots:
    /// Called when the user asks to remove the bookmark. Will close the dialog after removal
    void onRemoveBookmark();

    /// Called when the user is done potentially modifying the bookmark name/folder/etc. Saves info and closes the dialog
    void saveAndClose();

private:
    /// UI elements
    Ui::AddBookmarkDialog *ui;

    /// Bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// URL of current bookmark shown in the UI (bookmarks are referenced by their unique URL values in bookmark manager)
    QString m_currentUrl;
};

#endif // ADDBOOKMARKDIALOG_H
