#ifndef GENERALTAB_H
#define GENERALTAB_H

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
    explicit GeneralTab(QWidget *parent = nullptr);
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

    /// Sets the index of the startup mode combo box (0 = home page, 1 = blank page, 2 = restore session)
    void setStartupIndex(int index);

    /// Returns true if set to load the home page on new tabs, false if blank page should be loaded instead
    bool doNewTabsLoadHomePage() const;

    /// Sets the radio button associated with new tabs loading the home page either on or off, depending on value
    void setNewTabsLoadHomePage(bool value);

private slots:
    /// Toggles the active/inactive state of the line edit associated with the download directory
    void toggleLineEditDownloadDir();

private:
    Ui::GeneralTab *ui;
};

#endif // GENERALTAB_H
