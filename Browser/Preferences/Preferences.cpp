#include "Preferences.h"
#include "ui_Preferences.h"

#include "AdBlockManager.h"
#include "BrowserApplication.h"
#include "HistoryManager.h"
#include "Settings.h"
#include "UserScriptManager.h"
#include <QDir>

Preferences::Preferences(std::shared_ptr<Settings> settings, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Preferences),
    m_settings(settings)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Preferences::onCloseWithSave);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Preferences::close);

    connect(ui->tabPrivacy, &PrivacyTab::clearHistoryRequested, this, &Preferences::clearHistoryRequested);
    connect(ui->tabPrivacy, &PrivacyTab::viewHistoryRequested, this, &Preferences::viewHistoryRequested);
}

Preferences::~Preferences()
{
    delete ui;
}

void Preferences::loadSettings()
{
    ui->tabGeneral->setDownloadDirectory(m_settings->getValue(QLatin1String("DownloadDir")).toString());
    ui->tabGeneral->setDownloadAskBehavior(m_settings->getValue(QLatin1String("AskWhereToSaveDownloads")).toBool());
    ui->tabGeneral->setHomePage(m_settings->getValue(QLatin1String("HomePage")).toString());
    ui->tabGeneral->setStartupIndex(m_settings->getValue(QLatin1String("StartupMode")).toInt());
    ui->tabGeneral->setNewTabsLoadHomePage(m_settings->getValue(QLatin1String("NewTabsLoadHomePage")).toBool());

    ui->tabContent->toggleAdBlock(m_settings->getValue(QLatin1String("AdBlockPlusEnabled")).toBool());
    ui->tabContent->toggleAutoLoadImages(m_settings->getValue(QLatin1String("AutoLoadImages")).toBool());
    ui->tabContent->togglePlugins(m_settings->getValue(QLatin1String("EnablePlugins")).toBool());
    ui->tabContent->togglePopupBlock(!m_settings->getValue(QLatin1String("EnableJavascriptPopups")).toBool());
    ui->tabContent->toggleJavaScript(m_settings->getValue(QLatin1String("EnableJavascript")).toBool());
    ui->tabContent->toggleUserScripts(m_settings->getValue(QLatin1String("UserScriptsEnabled")).toBool());
    ui->tabContent->setDefaultFont(m_settings->getValue(QLatin1String("StandardFont")).toString());
    ui->tabContent->setSerifFont(m_settings->getValue(QLatin1String("SerifFont")).toString());
    ui->tabContent->setSansSerifFont(m_settings->getValue(QLatin1String("SansSerifFont")).toString());
    ui->tabContent->setCursiveFont(m_settings->getValue(QLatin1String("CursiveFont")).toString());
    ui->tabContent->setFantasyFont(m_settings->getValue(QLatin1String("FantasyFont")).toString());
    ui->tabContent->setFixedFont(m_settings->getValue(QLatin1String("FixedFont")).toString());
    ui->tabContent->setStandardFontSize(m_settings->getValue(QLatin1String("StandardFontSize")).toInt());
    ui->tabContent->setFixedFontSize(m_settings->getValue(QLatin1String("FixedFontSize")).toInt());

    ui->tabPrivacy->setHistoryStoragePolicy(static_cast<HistoryStoragePolicy>(m_settings->getValue(QLatin1String("HistoryStoragePolicy")).toInt()));
    ui->tabPrivacy->setCookiesEnabled(m_settings->getValue(QLatin1String("EnableCookies")).toBool());
    ui->tabPrivacy->setCookiesDeleteWithSession(m_settings->getValue(QLatin1String("CookiesDeleteWithSession")).toBool());
    ui->tabPrivacy->setThirdPartyCookiesEnabled(m_settings->getValue(QLatin1String("EnableThirdPartyCookies")).toBool());
}

void Preferences::onCloseWithSave()
{
    if (!m_settings)
        return;

    // Fetch preferences in the General tab
    QString currItem = ui->tabGeneral->getHomePage();
    if (!currItem.isEmpty())
        m_settings->setValue(QLatin1String("HomePage"), currItem);

    currItem = ui->tabGeneral->getDownloadDirectory();
    QDir downDir(currItem);
    if (downDir.exists())
        m_settings->setValue(QLatin1String("DownloadDir"), currItem);

    m_settings->setValue(QLatin1String("AskWhereToSaveDownloads"), ui->tabGeneral->getDownloadAskBehavior());

    m_settings->setValue(QLatin1String("StartupMode"), ui->tabGeneral->getStartupIndex());
    m_settings->setValue(QLatin1String("NewTabsLoadHomePage"), ui->tabGeneral->doNewTabsLoadHomePage());

    // Save preferences in Content tab
    AdBlockManager::instance().setEnabled(ui->tabContent->isAdBlockEnabled());
    m_settings->setValue(QLatin1String("AdBlockPlusEnabled"), ui->tabContent->isAdBlockEnabled());
    m_settings->setValue(QLatin1String("AutoLoadImages"), ui->tabContent->isAutoLoadImagesEnabled());
    m_settings->setValue(QLatin1String("EnablePlugins"), ui->tabContent->arePluginsEnabled());
    m_settings->setValue(QLatin1String("EnableJavascriptPopups"), ui->tabContent->arePopupsEnabled());
    m_settings->setValue(QLatin1String("EnableJavascript"), ui->tabContent->isJavaScriptEnabled());
    m_settings->setValue(QLatin1String("UserScriptsEnabled"), ui->tabContent->areUserScriptsEnabled());
    sBrowserApplication->getUserScriptManager()->setEnabled(ui->tabContent->areUserScriptsEnabled());

    // Save font choices
    m_settings->setValue(QLatin1String("StandardFont"), ui->tabContent->getDefaultFont());
    m_settings->setValue(QLatin1String("SerifFont"), ui->tabContent->getSerifFont());
    m_settings->setValue(QLatin1String("SansSerifFont"), ui->tabContent->getSansSerifFont());
    m_settings->setValue(QLatin1String("CursiveFont"), ui->tabContent->getCursiveFont());
    m_settings->setValue(QLatin1String("FantasyFont"), ui->tabContent->getFantasyFont());
    m_settings->setValue(QLatin1String("FixedFont"), ui->tabContent->getFixedFont());

    m_settings->setValue(QLatin1String("StandardFontSize"), ui->tabContent->getStandardFontSize());
    m_settings->setValue(QLatin1String("FixedFontSize"), ui->tabContent->getFixedFontSize());

    // Save privacy choices
    m_settings->setValue(QLatin1String("HistoryStoragePolicy"), QVariant::fromValue(static_cast<int>(ui->tabPrivacy->getHistoryStoragePolicy())));
    m_settings->setValue(QLatin1String("EnableCookies"), ui->tabPrivacy->areCookiesEnabled());
    m_settings->setValue(QLatin1String("CookiesDeleteWithSession"), ui->tabPrivacy->areCookiesDeletedWithSession());
    m_settings->setValue(QLatin1String("EnableThirdPartyCookies"), ui->tabPrivacy->areThirdPartyCookiesEnabled());

    // Apply web settings to web brower engine
    m_settings->applyWebSettings();

    close();
}
