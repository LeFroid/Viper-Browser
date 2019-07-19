#ifndef GENERALTAB_H
#define GENERALTAB_H

#include "Settings.h"
#include <QWidget>

namespace Ui {
    class GeneralTab;
}

/**
 * @class GeneralTab
 * @brief Represents the configuration settings in the General tab of the
 *        \ref Preferences widget, including browser start behavior and
 *        download settings
 */
class GeneralTab : public QWidget
{
    Q_OBJECT

public:
    /// Tab constructor
    explicit GeneralTab(QWidget *parent = nullptr);

    /// Destructor
    ~GeneralTab();

    /**
     * @brief setDownloadAskBehavior Sets the behavior of browser downloads
     * @param alwaysAsk If true, browser will always ask where to save a download, and what to
     *                  name the download on the filesystem. If false, will default to the external
     *                  download name in the user-set download directory.
     */
    void setDownloadAskBehavior(bool alwaysAsk);

    /// Returns the behavior of browser downloads. If return true, browser will always ask where
    /// to save a file and what to name it. If returns false, will default to external file's name
    /// in the user-set download directory
    bool getDownloadAskBehavior() const;

    /// Returns the download directory as specified in the \ref FileChooserLineEdit
    QString getDownloadDirectory() const;

    /// Sets the download directory to be displayed in the download section
    void setDownloadDirectory(const QString &path);

    /// Returns the home page as specified in the UI
    QString getHomePage() const;

    /// Sets the home page to be displayed in the general tab
    void setHomePage(const QString &homePage);

    /// Returns the index of the item in the startup mode combo box. Can be cast to a \ref StartupMode value
    int getStartupIndex() const;

    /// Sets the index of the startup mode combo box (0 = home page, 1 = blank page, 2 = welcome page, 3 = restore session)
    void setStartupIndex(int index);

    /// Returns the type of page that is loaded by default on new tabs
    NewTabType getNewTabPageType() const;

    /// Sets the radio button associated with new tabs' default page type
    void setNewTabPageType(NewTabType value);

    /// Returns true if all new tabs should be opened without changing focus from the current tab, false if sometimes new tabs
    /// can take focus
    bool openAllTabsInBackground() const;

    /// Sets the behavior of new tabs opening in the background. If value is true, all tabs are opened in background. Otherwise,
    /// some tabs will take focus when opened.
    void setAllTabsOpenInBackground(bool value);

private Q_SLOTS:
    /// Toggles the active/inactive state of the line edit associated with the download directory
    void toggleLineEditDownloadDir();

private:
    Ui::GeneralTab *ui;
};

#endif // GENERALTAB_H
