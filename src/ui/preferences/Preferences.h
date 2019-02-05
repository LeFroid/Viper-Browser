#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "ServiceLocator.h"

#include <memory>
#include <QWidget>

namespace Ui {
    class Preferences;
}

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
    /// Constructs the preferences widget given a pointer to the global settings object and an optional parent
    explicit Preferences(Settings *settings, ViperServiceLocator &serviceLocator, QWidget *parent = nullptr);

    /// Preferences destructor
    ~Preferences();

signals:
    /// Emitted when the user requests to clear their browsing history.
    void clearHistoryRequested();

    /// Emitted when the user requests to view their browsing history.
    void viewHistoryRequested();

    /// Emitted when the user wants to view all login information saved by the auto fill system
    void viewSavedCredentialsRequested();

    /// Emitted when the user wants to view the website logins that have been forbidden from being saved
    void viewAutoFillExceptionsRequested();

private slots:
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

    /// Reference to the service locator / registry
    ViperServiceLocator &m_serviceLocator;
};

#endif // PREFERENCES_H
