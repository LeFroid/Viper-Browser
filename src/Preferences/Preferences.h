#ifndef PREFERENCES_H
#define PREFERENCES_H

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
    explicit Preferences(Settings *settings, QWidget *parent = nullptr);

    /// Preferences destructor
    ~Preferences();

signals:
    /// Emitted when the user requests to clear their browsing history.
    void clearHistoryRequested();

    /// Emitted when the user requests to view their browsing history.
    void viewHistoryRequested();

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
};

#endif // PREFERENCES_H
