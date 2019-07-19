#ifndef BOOKMARKDIALOG_H
#define BOOKMARKDIALOG_H

#include "BookmarkManager.h"
#include <QDialog>
#include <QString>
#include <QUrl>

class BookmarkManager;

namespace Ui {
    class BookmarkDialog;
}

/**
 * @class BookmarkDialog
 * @brief Shown when the user adds a new bookmark to their collection through a menu
 *        option in the main window. Also used for editing and/or removing an existing
 *        bookmark. Allows the user to change the bookmark name and its parent folder.
 * @ingroup Bookmarks
 */
class BookmarkDialog : public QDialog
{
    Q_OBJECT

public:
    /// Constructs the dialog given a pointer to the bookmark manager and optionally a parent widget
    explicit BookmarkDialog(BookmarkManager *bookmarkMgr, QWidget *parent = 0);

    /// Dialog destructor
    ~BookmarkDialog();

    /// Aligns the bookmark dialog beneath the URL bar and towards its right side, showing the dialog after alignment
    void alignAndShow(const QRect &windowGeom, const QRect &toolbarGeom, const QRect &urlBarGeom);

    /// Sets the text of the dialog's header. Should be "Bookmark Added" or "Bookmark"
    void setDialogHeader(const QString &text);

    /// Sets the information fields of the newly added bookmark that will be displayed in the dialog
    void setBookmarkInfo(const QString &bookmarkName, const QUrl &bookmarkUrl, BookmarkNode *parentFolder = nullptr);

private Q_SLOTS:
    /// Called when the user asks to remove the bookmark. Will close the dialog after removal
    void onRemoveBookmark();

    /// Called when the user is done potentially modifying the bookmark name/folder/etc. Saves info and closes the dialog
    void saveAndClose();

private:
    /// UI elements
    Ui::BookmarkDialog *ui;

    /// Bookmark manager
    BookmarkManager *m_bookmarkManager;

    /// URL of current bookmark shown in the UI (bookmarks are referenced by their unique URL values in bookmark manager)
    QUrl m_currentUrl;
};

#endif // BOOKMARKDIALOG_H
