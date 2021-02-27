#ifndef ADBLOCKBUTTON_H
#define ADBLOCKBUTTON_H

#include <QIcon>
#include <QTimer>
#include <QToolButton>

namespace adblock {
    class AdBlockManager;
}

class MainWindow;
class Settings;

/**
 * @class AdBlockButton
 * @brief Acts as a visual indicator to the number of ads that are being blocked
 *        on a \ref WebPage. Displayed in the \ref NavigationToolBar
 */
class AdBlockButton : public QToolButton
{
    Q_OBJECT

public:
    /// Constructs the AdBlockButton with a given parent
    explicit AdBlockButton(bool isDarkTheme, QWidget *parent = nullptr);

    /// Sets the pointer to the advertisement blocking system manager
    void setAdBlockManager(adblock::AdBlockManager *adBlockManager);

    /// Sets the pointer to the application-level settings handler
    void setSettings(Settings *settings);

Q_SIGNALS:
    /// Emitted when the user wants to enable or disable the ad block system
    void toggleAdBlockEnabledRequest();

    /// Emitted when the user requests to view and/or manage their filter subscriptions
    void manageSubscriptionsRequest();

    /// Emitted when the user requests to view the ad block system logs
    void viewLogsRequest();

public Q_SLOTS:
    /// Updates the region of the button's icon containing the number of ads being blocked on the active page
    void updateCount();

protected:
    /// Displays a menu with various options related to the ad-block system, when the user right clicks on this button
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    /// Advertisement blocking system manager
    adblock::AdBlockManager *m_adBlockManager;

    /// Application settings
    Settings *m_settings;

    /// Pointer to the parent window
    MainWindow *m_mainWindow;

    /// Base icon shown in the button
    QIcon m_icon;

    /// Timer that periodically calls the updateCount() method
    QTimer m_timer;

    /// The last reported number of ads that were blocked on a page when the updateCount() slot was called
    int m_lastCount;
};

#endif // ADBLOCKBUTTON_H
