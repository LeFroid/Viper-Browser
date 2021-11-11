#ifndef COOKIEWIDGET_H
#define COOKIEWIDGET_H

#include <QWidget>

namespace Ui {
class CookieWidget;
}

class CookieJar;
class CookieModifyDialog;
class QWebEngineProfile;

/**
 * @class CookieWidget
 * @brief Displays the user's cookies and allows for modification, addition
 *        and deletion of cookies.
 */
class CookieWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the cookie widget
    explicit CookieWidget(QWebEngineProfile *profile);
    ~CookieWidget();

    /// Resets the checkbox states in the table view
    void resetUI();

    /// Filters the cookie table for those that match the given host
    void searchForHost(const QString &hostname);

protected:
    /// Called to adjust the proportions of the columns belonging to the table views
    virtual void resizeEvent(QResizeEvent *event) override;

private Q_SLOTS:
    /// Searches for any subset of cookies that match a search parameter given by the user
    void searchCookies();

    /// Called when a cookie in the top table is clicked
    void onCookieClicked(const QModelIndex &index);

    /// Called when the "New Cookie" push button has been clicked
    void onNewCookieClicked();

    /// Called when the "Edit Cookie" push button has been clicked
    void onEditCookieClicked();

    /// Called when the delete cookie(s) push button has been clicked
    void onDeleteClicked();

    /// Called when the cookie dialog has been closed with the given result
    void onCookieDialogFinished(int result);

private:
    /// User interface
    Ui::CookieWidget *ui;

    /// Dialog for adding or modifying cookies
    CookieModifyDialog *m_cookieDialog;

    /// Web engine profile
    QWebEngineProfile *m_profile;

    /// True if the dialog was activated in edit mode, false if dialog is in new cookie mode
    bool m_dialogEditMode;
};

#endif // COOKIEWIDGET_H
