#ifndef PRIVACYTAB_H
#define PRIVACYTAB_H

#include "HistoryManager.h"

#include <QWidget>

namespace Ui {
    class PrivacyTab;
}

/**
 * @class PrivacyTab
 * @brief Contains the web browser settings for privacy-related matters including
 *        history and cookie settings
 */
class PrivacyTab : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the privacy tab
    explicit PrivacyTab(QWidget *parent = nullptr);

    /// Privacy tab destructor
    ~PrivacyTab();

    /// Returns the user set history storage policy
    HistoryStoragePolicy getHistoryStoragePolicy() const;

    /// Returns true if cookies are to be enabled, false if disabled
    bool areCookiesEnabled() const;

    /// Returns true if cookies are to be deleted at the end of each browsing session, false if else
    bool areCookiesDeletedWithSession() const;

    /// Returns true if third party cookies are allowed, false if they should be disabled or filtered
    bool areThirdPartyCookiesEnabled() const;

signals:
    /// Emitted when the user requests to clear their browsing history.
    void clearHistoryRequested();

    /// Emitted when the user clicks the "view history" button.
    void viewHistoryRequested();

public slots:
    /// Sets the item in the history policy combo box to the one reflecting the given policy
    void setHistoryStoragePolicy(HistoryStoragePolicy policy);

    /// Sets the cookie storage setting. If the value passed is true, cookies will be enabled, otherwise they will not be stored
    void setCookiesEnabled(bool value);

    /// Sets the behavior of cookie storage. If the value passed is true, all cookies will be deleted when the browsing session is over.
    /// Otherwise, cookies will be stored until they expire.
    void setCookiesDeleteWithSession(bool value);

    /// Sets the behavior for acceptance of third-party cookies. If the value passed is true, third party cookies will be allowed.
    /// Otherwise, they will be filtered out.
    void setThirdPartyCookiesEnabled(bool value);

private:
    /// Pointer to the UI form elements
    Ui::PrivacyTab *ui;
};

#endif // PRIVACYTAB_H
