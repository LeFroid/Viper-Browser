#ifndef EXEMPTTHIRDPARTYCOOKIEDIALOG_H
#define EXEMPTTHIRDPARTYCOOKIEDIALOG_H

#include <QSet>
#include <QUrl>
#include <QWidget>

namespace Ui {
    class ExemptThirdPartyCookieDialog;
}

class QAbstractButton;

/**
 * @class ExemptThirdPartyCookieDialog
 * @brief Allows the user to view and manage exempt third party hosts that are allowed to
 *        set cookies.
 */
class ExemptThirdPartyCookieDialog : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the dialog
    explicit ExemptThirdPartyCookieDialog(QWidget *parent = nullptr);

    /// Dialog destructor
    ~ExemptThirdPartyCookieDialog();

private:
    /// Loads the exempt host information into the list widget
    void loadHosts();

private slots:
    /// Called when the "add website" button is clicked
    void onAddHostButtonClicked();

    /// Called when the "remove website" button is clicked
    void onRemoveHostButtonClicked();

    /// Called when the "remove all websites" button is clicked
    void onRemoveAllHostsButtonClicked();

    /// Called when a button from the dialog button box is clicked
    void onDialogButtonClicked(QAbstractButton *button);

private:
    /// Pointer to the UI elements
    Ui::ExemptThirdPartyCookieDialog *ui;

    /// Set of hosts the user chose to add to the exempt list
    QSet<QUrl> m_hostsToAdd;

    /// Set of hosts the user chose to delete from the exempt list
    QSet<QUrl> m_hostsToDelete;
};

#endif // EXEMPTTHIRDPARTYCOOKIEDIALOG_H
