#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <memory>
#include <QWidget>

namespace Ui {
    class Preferences;
}

class CookieJar;
class Settings;

/**
 * @class Preferences
 * @brief Graphical representation of the user's browser settings. Allows for
 *        modification of various settings in the browser
 */
class Preferences : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the preferences widget given a pointer to the global settings object,
    /// the user's cookie jar, and an optional parent
    explicit Preferences(Settings *settings, CookieJar *cookieJar, QWidget *parent = nullptr);

    /// Preferences destructor
    ~Preferences();

Q_SIGNALS:
    /// Emitted when the user requests to clear their browsing history.
    void clearHistoryRequested();

    /// Emitted when the user requests to view their browsing history.
    void viewHistoryRequested();

    /// Emitted when the user wants to view all login information saved by the auto fill system
    void viewSavedCredentialsRequested();

    /// Emitted when the user wants to view the website logins that have been forbidden from being saved
    void viewAutoFillExceptionsRequested();

private Q_SLOTS:
    /// Called when
    void onCloseWithSave();

private:
    /// Sets the UI values corresponding to browser settings
    void loadSettings();

private:
    /// User interface
    Ui::Preferences *ui;

    /// Pointer to the global browser settings
    Settings *m_settings;
};

#endif // PREFERENCES_H
