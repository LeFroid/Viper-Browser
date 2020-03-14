#ifndef WELCOMEWINDOW_H
#define WELCOMEWINDOW_H

#include <vector>

#include <QUrl>
#include <QWizard>

namespace adblock
{
    class AdBlockManager;
}

class AdBlockPage;
class Settings;

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QListWidget;

/**
 * @class WelcomeWindow
 * @brief This class walks the user through some initial profile
 *        setup questions, including choice of ad-block subscriptions,
 *        a home page, default search engine, etc.
 */
class WelcomeWindow : public QWizard
{
    Q_OBJECT

public:
    /// Constructs the welcome window, with a pointer to the profile settings, and a pointer to the adblock manager
    explicit WelcomeWindow(Settings *settings, adblock::AdBlockManager *adBlockManager);

    /// Invoked when the dialog is accepted or submitted by the user
    void accept() override;

private:
    /// Points to the user profile settings container. This will be updated if
    /// the user fills out the wizard inputs and submits their preferences.
    Settings *m_settings;

    /// Combo box pointer is stored here for special handling of the search engine preference
    QComboBox *m_comboBoxSearchEngine;

    /// Advertisement blocking manager
    adblock::AdBlockManager *m_adBlockManager;

    /// Advertisement blocking page
    AdBlockPage *m_adBlockPage;
};

/**
 * @class IntroductionPage
 * @brief This is the first page of the welcome window. It explains
 *        the purpose of the dialog and what inputs the user will
 *        be asked to provide in the subsequent pages.
 */
class IntroductionPage : public QWizardPage
{
    Q_OBJECT

public:
    /// Constructs the introduction page
    IntroductionPage(QWidget *parent = nullptr);

private:
    /// Contains the majority of text on the page
    QLabel *m_labelPageDescription;
};

/**
 * @class GeneralSettingsPage
 * @brief This page takes the user's inputs for home page,
 *        search engine, and browser startup behavior.
 */
class GeneralSettingsPage : public QWizardPage
{
    Q_OBJECT

public:
    /// Constructs the general settings page
    GeneralSettingsPage(QWidget *parent = nullptr);

    /// Returns a pointer to the search engine dropdown
    QComboBox *getSearchEngineComboBox() const;

private:
    /// Loads the search engines from the \ref SearchEngineManager and into
    /// the combo box
    void loadSearchEngines();

private:
    /// Combo box containing the startup behavior options
    QComboBox *m_comboBoxStartup;

    /// Input widget for the home page URL
    QLineEdit *m_lineEditHomePage;

    /// Dropdown containing choice of default search engine. The
    /// user can add new search engines through the \ref SearchTab
    /// in the \ref Preferences window
    QComboBox *m_comboBoxSearchEngine;

    /// Checkbox for the preference to enable plugins like PPAPI and Pdfium (if checked)
    QCheckBox *m_checkboxEnablePlugins;
};

class AdBlockPage : public QWizardPage
{
    Q_OBJECT

public:
    /// Constructs the advertisement blocking page
    AdBlockPage(QWidget *parent = nullptr);

    /// Returns the filter lists to which the user will subscribe
    const std::vector<QUrl> getSelectedSubscriptions() const;

private:
    /// List of potential subscriptions
    QListWidget *m_subscriptionList;
};

#endif // WELCOMEWINDOW_H
