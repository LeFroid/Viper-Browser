#include "welcome_window/WelcomeWindow.h"

#include "AdBlockManager.h"
#include "BrowserSetting.h"
#include "RecommendedSubscriptions.h"
#include "SearchEngineManager.h"
#include "Settings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QVBoxLayout>

#include <QDebug>

WelcomeWindow::WelcomeWindow(Settings *settings, adblock::AdBlockManager *adBlockManager) :
    QWizard(nullptr),
    m_settings(settings),
    m_comboBoxSearchEngine(nullptr),
    m_adBlockManager(adBlockManager),
    m_adBlockPage(nullptr)
{
    setWindowTitle(tr("Profile Setup"));

    addPage(new IntroductionPage);

    GeneralSettingsPage *generalPage = new GeneralSettingsPage;
    m_comboBoxSearchEngine = generalPage->getSearchEngineComboBox();
    addPage(generalPage);

    m_adBlockPage = new AdBlockPage;
    addPage(m_adBlockPage);
}

void WelcomeWindow::accept()
{
    m_settings->setValue(BrowserSetting::StartupMode, field(QLatin1String("StartupMode")));
    m_settings->setValue(BrowserSetting::HomePage, field(QLatin1String("HomePage")));
    m_settings->setValue(BrowserSetting::EnablePlugins, field(QLatin1String("PlugIns")));

    if (m_comboBoxSearchEngine != nullptr)
        SearchEngineManager::instance().setDefaultSearchEngine(m_comboBoxSearchEngine->itemText(m_comboBoxSearchEngine->currentIndex()));

    if (m_adBlockManager)
    {
        const std::vector<QUrl> adblockSubscriptions = m_adBlockPage->getSelectedSubscriptions();
        for (const QUrl &subscription : adblockSubscriptions)
        {
            m_adBlockManager->installSubscription(subscription);
        }
    }

    QDialog::accept();
}

IntroductionPage::IntroductionPage(QWidget *parent) :
    QWizardPage(parent),
    m_labelPageDescription(nullptr)
{
    setTitle(tr("Introduction"));

    m_labelPageDescription
            = new QLabel(tr("Thank you for installing Viper Browser! The following pages will "
                            "ask you some questions, which will allow us to custom-tailor the "
                            "browser for a more enjoyable experience."));
    m_labelPageDescription->setWordWrap(true);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->addWidget(m_labelPageDescription);
    setLayout(vboxLayout);
}

GeneralSettingsPage::GeneralSettingsPage(QWidget *parent) :
    QWizardPage(parent),
    m_comboBoxStartup(new QComboBox(this)),
    m_lineEditHomePage(new QLineEdit(this)),
    m_comboBoxSearchEngine(new QComboBox(this)),
    m_checkboxEnablePlugins(new QCheckBox(tr("Enable browser plug-ins (eg Flash, Pdfium)"), this))
{
    m_comboBoxStartup->addItem(tr("Show my home page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadHomePage)));
    m_comboBoxStartup->addItem(tr("Show a blank page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadBlankPage)));
    m_comboBoxStartup->addItem(tr("Show the welcome page"), QVariant::fromValue(static_cast<int>(StartupMode::LoadNewTabPage)));
    m_comboBoxStartup->addItem(tr("Show my tabs from last time"), QVariant::fromValue(static_cast<int>(StartupMode::RestoreSession)));

    m_lineEditHomePage->setText(QLatin1String("https://startpage.com/"));

    loadSearchEngines();

    registerField(QLatin1String("StartupMode"), m_comboBoxStartup);
    registerField(QLatin1String("HomePage"), m_lineEditHomePage);
    registerField(QLatin1String("PlugIns"), m_checkboxEnablePlugins);

    QFormLayout *formLayout = new QFormLayout;

    formLayout->addRow(tr("When the browser starts:"), m_comboBoxStartup);
    formLayout->addRow(tr("Home page:"), m_lineEditHomePage);
    formLayout->addRow(tr("Default search engine:"), m_comboBoxSearchEngine);
    formLayout->addRow(m_checkboxEnablePlugins);

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

AdBlockPage::AdBlockPage(QWidget *parent) :
    QWizardPage(parent),
    m_subscriptionList(new QListWidget(this))
{
    setTitle(tr("Advertisement Blocking"));
    setSubTitle(tr("Select from any of the below featured subscription lists, or ignore to "
                   "disable advertisement blocking."));

    adblock::RecommendedSubscriptions recommendations;

    // Populate the list widget
    for (const std::pair<QString, QUrl> &sub : recommendations)
    {
        QListWidgetItem* listItem = new QListWidgetItem(sub.first, m_subscriptionList);
        listItem->setFlags(listItem->flags() | Qt::ItemIsUserCheckable);
        listItem->setCheckState(Qt::Unchecked);
        listItem->setData(Qt::UserRole, sub.second);
    }

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->addWidget(m_subscriptionList);
    setLayout(vboxLayout);
}

const std::vector<QUrl> AdBlockPage::getSelectedSubscriptions() const
{
    std::vector<QUrl> selection;

    int numItems = m_subscriptionList->count();
    for (int i = 0; i < numItems; ++i)
    {
        QListWidgetItem *item = m_subscriptionList->item(i);
        if (item == nullptr)
            continue;

        if (item->checkState() == Qt::Checked)
            selection.push_back(item->data(Qt::UserRole).toUrl());
    }

    return selection;
}
