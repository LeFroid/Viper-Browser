#include "welcome_window/WelcomeWindow.h"

#include "BrowserSetting.h"
#include "SearchEngineManager.h"
#include "Settings.h"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QString>
#include <QVBoxLayout>

#include <QDebug>

//TODO: page for adblock/ublock subscriptions, option to enable/disable ppapi plugins in perhaps the general page

WelcomeWindow::WelcomeWindow(Settings *settings) :
    QWizard(nullptr),
    m_settings(settings),
    m_comboBoxSearchEngine(nullptr)
{
    setWindowTitle(tr("Profile Setup"));

    addPage(new IntroductionPage);

    GeneralSettingsPage *generalPage = new GeneralSettingsPage;
    m_comboBoxSearchEngine = generalPage->getSearchEngineComboBox();
    addPage(generalPage);
}

void WelcomeWindow::accept()
{
    qDebug() << "WelcomeWindow::accept()";

    m_settings->setValue(BrowserSetting::StartupMode, field(QLatin1String("StartupMode")));
    m_settings->setValue(BrowserSetting::HomePage, field(QLatin1String("HomePage")));

    if (m_comboBoxSearchEngine != nullptr)
        SearchEngineManager::instance().setDefaultSearchEngine(m_comboBoxSearchEngine->itemText(m_comboBoxSearchEngine->currentIndex()));
}

IntroductionPage::IntroductionPage(QWidget *parent) :
    QWizardPage(parent),
    m_labelPageDescription(nullptr)
{
    setTitle(tr("Introduction"));

    m_labelPageDescription
            = new QLabel(tr("Thank you for installing Viper Browser! The following pages will "
                            "ask you to set your profile settings for an optimal browsing experience. "
                            "This includes choice of search engine, home page, advertisement blocking "
                            "subscriptions (\"filter lists\"), and optional plugins like the Flash player."));
    m_labelPageDescription->setWordWrap(true);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->addWidget(m_labelPageDescription);
    setLayout(vboxLayout);
}

GeneralSettingsPage::GeneralSettingsPage(QWidget *parent) :
    QWizardPage(parent),
    m_comboBoxStartup(new QComboBox(this)),
    m_lineEditHomePage(new QLineEdit(this)),
    m_comboBoxSearchEngine(new QComboBox(this))
{
    m_comboBoxStartup->addItem(tr("Show my home page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadHomePage)));
    m_comboBoxStartup->addItem(tr("Show a blank page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadBlankPage)));
    m_comboBoxStartup->addItem(tr("Show the welcome page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadNewTabPage)));
    m_comboBoxStartup->addItem(tr("Show my tabs from last time"), QVariant::fromValue(static_cast<int>(StartupMode::RestoreSession)));

    m_lineEditHomePage->setText(QLatin1String("https://startpage.com/"));

    loadSearchEngines();

    registerField(QLatin1String("StartupMode"), m_comboBoxStartup);
    registerField(QLatin1String("HomePage"), m_lineEditHomePage);

    QFormLayout *formLayout = new QFormLayout;

    formLayout->addRow(tr("When the browser starts:"), m_comboBoxStartup);
    formLayout->addRow(tr("Home page:"), m_lineEditHomePage);
    formLayout->addRow(tr("Default search engine:"), m_comboBoxSearchEngine);

    setLayout(formLayout);
}

QComboBox *GeneralSettingsPage::getSearchEngineComboBox() const
{
    return m_comboBoxSearchEngine;
}

void GeneralSettingsPage::loadSearchEngines()
{
    SearchEngineManager &manager = SearchEngineManager::instance();

    QList<QString> searchEngines = manager.getSearchEngineNames();
    const QString &defaultEngine = manager.getDefaultSearchEngine();

    int defaultEngineIdx = searchEngines.indexOf(defaultEngine);

    for (const QString &engine : searchEngines)
        m_comboBoxSearchEngine->addItem(engine);

    m_comboBoxSearchEngine->setCurrentIndex(defaultEngineIdx);
}
