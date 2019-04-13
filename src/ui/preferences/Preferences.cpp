#include "Preferences.h"
#include "ui_Preferences.h"

#include "AppInitSettings.h"
#include "CookieJar.h"
#include "HistoryManager.h"
#include "Settings.h"

#include <QDir>

Preferences::Preferences(Settings *settings, CookieJar *cookieJar, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Preferences),
    m_settings(settings)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &Preferences::onCloseWithSave);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &Preferences::close);

    connect(ui->tabPrivacy, &PrivacyTab::clearHistoryRequested, this, &Preferences::clearHistoryRequested);
    connect(ui->tabPrivacy, &PrivacyTab::viewHistoryRequested, this, &Preferences::viewHistoryRequested);

    ui->tabPrivacy->setCookieJar(cookieJar);
    connect(ui->tabPrivacy, &PrivacyTab::viewSavedCredentialsRequested, this, &Preferences::viewSavedCredentialsRequested);
    connect(ui->tabPrivacy, &PrivacyTab::viewAutoFillExceptionsRequested, this, &Preferences::viewAutoFillExceptionsRequested);

    loadSettings();
}

Preferences::~Preferences()
{
    delete ui;
}

void Preferences::loadSettings()
{
    ui->tabGeneral->setDownloadDirectory(m_settings->getValue(BrowserSetting::DownloadDir).toString());
    ui->tabGeneral->setDownloadAskBehavior(m_settings->getValue(BrowserSetting::AskWhereToSaveDownloads).toBool());
    ui->tabGeneral->setHomePage(m_settings->getValue(BrowserSetting::HomePage).toString());
    ui->tabGeneral->setStartupIndex(m_settings->getValue(BrowserSetting::StartupMode).toInt());
    ui->tabGeneral->setNewTabPageType(static_cast<NewTabType>(m_settings->getValue(BrowserSetting::NewTabPage).toInt()));
    ui->tabGeneral->setAllTabsOpenInBackground(m_settings->getValue(BrowserSetting::OpenAllTabsInBackground).toBool());

    ui->tabContent->toggleAdBlock(m_settings->getValue(BrowserSetting::AdBlockPlusEnabled).toBool());
    ui->tabContent->toggleAnimatedScrolling(m_settings->getValue(BrowserSetting::ScrollAnimatorEnabled).toBool());
    ui->tabContent->toggleAutoLoadImages(m_settings->getValue(BrowserSetting::AutoLoadImages).toBool());
    ui->tabContent->togglePlugins(m_settings->getValue(BrowserSetting::EnablePlugins).toBool());
    ui->tabContent->togglePopupBlock(!m_settings->getValue(BrowserSetting::EnableJavascriptPopups).toBool());
    ui->tabContent->toggleJavaScript(m_settings->getValue(BrowserSetting::EnableJavascript).toBool());
    ui->tabContent->toggleUserScripts(m_settings->getValue(BrowserSetting::UserScriptsEnabled).toBool());
    ui->tabContent->setDefaultFont(m_settings->getValue(BrowserSetting::StandardFont).toString());
    ui->tabContent->setSerifFont(m_settings->getValue(BrowserSetting::SerifFont).toString());
    ui->tabContent->setSansSerifFont(m_settings->getValue(BrowserSetting::SansSerifFont).toString());
    ui->tabContent->setCursiveFont(m_settings->getValue(BrowserSetting::CursiveFont).toString());
    ui->tabContent->setFantasyFont(m_settings->getValue(BrowserSetting::FantasyFont).toString());
    ui->tabContent->setFixedFont(m_settings->getValue(BrowserSetting::FixedFont).toString());
    ui->tabContent->setStandardFontSize(m_settings->getValue(BrowserSetting::StandardFontSize).toInt());
    ui->tabContent->setFixedFontSize(m_settings->getValue(BrowserSetting::FixedFontSize).toInt());

    ui->tabPrivacy->setAutoFillEnabled(m_settings->getValue(BrowserSetting::EnableAutoFill).toBool());
    ui->tabPrivacy->setHistoryStoragePolicy(static_cast<HistoryStoragePolicy>(m_settings->getValue(BrowserSetting::HistoryStoragePolicy).toInt()));
    ui->tabPrivacy->setCookiesEnabled(m_settings->getValue(BrowserSetting::EnableCookies).toBool());
    ui->tabPrivacy->setCookiesDeleteWithSession(m_settings->getValue(BrowserSetting::CookiesDeleteWithSession).toBool());
    ui->tabPrivacy->setThirdPartyCookiesEnabled(m_settings->getValue(BrowserSetting::EnableThirdPartyCookies).toBool());
    ui->tabPrivacy->setDoNotTrackEnabled(m_settings->getValue(BrowserSetting::SendDoNotTrack).toBool());

    AppInitSettings initSettings;
    if (initSettings.hasSetting(AppInitKey::ProcessModel))
        ui->tabAdvanced->setProcessModel(QString::fromStdString(initSettings.getValue(AppInitKey::ProcessModel)));
    ui->tabAdvanced->setGpuDisabled(initSettings.hasSetting(AppInitKey::DisableGPU));
}

void Preferences::onCloseWithSave()
{
    if (!m_settings)
        return;

    // Fetch preferences in the General tab
    QString currItem = ui->tabGeneral->getHomePage();
    if (!currItem.isEmpty())
        m_settings->setValue(BrowserSetting::HomePage, currItem);

    currItem = ui->tabGeneral->getDownloadDirectory();
    QDir downDir(currItem);
    if (downDir.exists())
        m_settings->setValue(BrowserSetting::DownloadDir, currItem);

    m_settings->setValue(BrowserSetting::AskWhereToSaveDownloads, ui->tabGeneral->getDownloadAskBehavior());

    m_settings->setValue(BrowserSetting::StartupMode, ui->tabGeneral->getStartupIndex());
    m_settings->setValue(BrowserSetting::NewTabPage, static_cast<int>(ui->tabGeneral->getNewTabPageType()));
    m_settings->setValue(BrowserSetting::OpenAllTabsInBackground, ui->tabGeneral->openAllTabsInBackground());

    // Save preferences in Content tab
    m_settings->setValue(BrowserSetting::AdBlockPlusEnabled, ui->tabContent->isAdBlockEnabled());
    m_settings->setValue(BrowserSetting::AutoLoadImages, ui->tabContent->isAutoLoadImagesEnabled());
    m_settings->setValue(BrowserSetting::EnablePlugins, ui->tabContent->arePluginsEnabled());
    m_settings->setValue(BrowserSetting::EnableJavascriptPopups, ui->tabContent->arePopupsEnabled());
    m_settings->setValue(BrowserSetting::EnableJavascript, ui->tabContent->isJavaScriptEnabled());
    m_settings->setValue(BrowserSetting::ScrollAnimatorEnabled, ui->tabContent->isAnimatedScrollEnabled());
    m_settings->setValue(BrowserSetting::UserScriptsEnabled, ui->tabContent->areUserScriptsEnabled());

    // Save font choices
    m_settings->setValue(BrowserSetting::StandardFont, ui->tabContent->getDefaultFont());
    m_settings->setValue(BrowserSetting::SerifFont, ui->tabContent->getSerifFont());
    m_settings->setValue(BrowserSetting::SansSerifFont, ui->tabContent->getSansSerifFont());
    m_settings->setValue(BrowserSetting::CursiveFont, ui->tabContent->getCursiveFont());
    m_settings->setValue(BrowserSetting::FantasyFont, ui->tabContent->getFantasyFont());
    m_settings->setValue(BrowserSetting::FixedFont, ui->tabContent->getFixedFont());

    m_settings->setValue(BrowserSetting::StandardFontSize, ui->tabContent->getStandardFontSize());
    m_settings->setValue(BrowserSetting::FixedFontSize, ui->tabContent->getFixedFontSize());

    // Save privacy choices
    m_settings->setValue(BrowserSetting::EnableAutoFill, ui->tabPrivacy->isAutoFillEnabled());
    m_settings->setValue(BrowserSetting::HistoryStoragePolicy, QVariant::fromValue(static_cast<int>(ui->tabPrivacy->getHistoryStoragePolicy())));
    m_settings->setValue(BrowserSetting::EnableCookies, ui->tabPrivacy->areCookiesEnabled());
    m_settings->setValue(BrowserSetting::CookiesDeleteWithSession, ui->tabPrivacy->areCookiesDeletedWithSession());
    m_settings->setValue(BrowserSetting::EnableThirdPartyCookies, ui->tabPrivacy->areThirdPartyCookiesEnabled());
    m_settings->setValue(BrowserSetting::SendDoNotTrack, ui->tabPrivacy->isDoNotTrackEnabled());

    // Application initialization-related settings (requires restart to take effect)
    AppInitSettings initSettings;
    std::string processModel = ui->tabAdvanced->getProcessModel().toStdString();
    if (!processModel.empty())
        initSettings.setValue(AppInitKey::ProcessModel, processModel);
    else if (initSettings.hasSetting(AppInitKey::ProcessModel))
        initSettings.removeSetting(AppInitKey::ProcessModel);
    if (ui->tabAdvanced->isGpuDisabled())
        initSettings.setValue(AppInitKey::DisableGPU, "--disable-gpu");
    else if (initSettings.hasSetting(AppInitKey::DisableGPU))
        initSettings.removeSetting(AppInitKey::DisableGPU);

    close();
}
