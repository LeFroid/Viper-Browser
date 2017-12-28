#include "Preferences.h"
#include "ui_Preferences.h"

#include "AdBlockManager.h"
#include "BrowserApplication.h"
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
}

Preferences::~Preferences()
{
    delete ui;
}

void Preferences::loadSettings()
{
    ui->tabGeneral->setDownloadDirectory(m_settings->getValue(QStringLiteral("DownloadDir")).toString());
    ui->tabGeneral->setDownloadAskBehavior(m_settings->getValue(QStringLiteral("AskWhereToSaveDownloads")).toBool());
    ui->tabGeneral->setHomePage(m_settings->getValue(QStringLiteral("HomePage")).toString());
    ui->tabGeneral->setStartupIndex(m_settings->getValue(QStringLiteral("StartupMode")).toInt());
    ui->tabGeneral->setNewTabsLoadHomePage(m_settings->getValue(QStringLiteral("NewTabsLoadHomePage")).toBool());

    ui->tabContent->toggleAdBlock(m_settings->getValue(QStringLiteral("AdBlockPlusEnabled")).toBool());
    ui->tabContent->toggleAutoLoadImages(m_settings->getValue(QStringLiteral("AutoLoadImages")).toBool());
    ui->tabContent->togglePlugins(m_settings->getValue(QStringLiteral("EnablePlugins")).toBool());
    ui->tabContent->togglePopupBlock(!m_settings->getValue(QStringLiteral("EnableJavascriptPopups")).toBool());
    ui->tabContent->toggleJavaScript(m_settings->getValue(QStringLiteral("EnableJavascript")).toBool());
    ui->tabContent->toggleUserScripts(m_settings->getValue(QStringLiteral("UserScriptsEnabled")).toBool());
    ui->tabContent->setDefaultFont(m_settings->getValue(QStringLiteral("StandardFont")).toString());
    ui->tabContent->setSerifFont(m_settings->getValue(QStringLiteral("SerifFont")).toString());
    ui->tabContent->setSansSerifFont(m_settings->getValue(QStringLiteral("SansSerifFont")).toString());
    ui->tabContent->setCursiveFont(m_settings->getValue(QStringLiteral("CursiveFont")).toString());
    ui->tabContent->setFantasyFont(m_settings->getValue(QStringLiteral("FantasyFont")).toString());
    ui->tabContent->setFixedFont(m_settings->getValue(QStringLiteral("FixedFont")).toString());
    ui->tabContent->setStandardFontSize(m_settings->getValue(QStringLiteral("StandardFontSize")).toInt());
    ui->tabContent->setFixedFontSize(m_settings->getValue(QStringLiteral("FixedFontSize")).toInt());
}

void Preferences::onCloseWithSave()
{
    if (!m_settings)
        return;

    // Fetch preferences in the General tab
    QString currItem = ui->tabGeneral->getHomePage();
    if (!currItem.isEmpty())
        m_settings->setValue(QStringLiteral("HomePage"), currItem);

    currItem = ui->tabGeneral->getDownloadDirectory();
    QDir downDir(currItem);
    if (downDir.exists())
        m_settings->setValue(QStringLiteral("DownloadDir"), currItem);

    m_settings->setValue(QStringLiteral("AskWhereToSaveDownloads"), ui->tabGeneral->getDownloadAskBehavior());

    m_settings->setValue(QStringLiteral("StartupMode"), ui->tabGeneral->getStartupIndex());
    m_settings->setValue(QStringLiteral("NewTabsLoadHomePage"), ui->tabGeneral->doNewTabsLoadHomePage());

    // Save preferences in Content tab
    AdBlockManager::instance().setEnabled(ui->tabContent->isAdBlockEnabled());
    m_settings->setValue(QStringLiteral("AdBlockPlusEnabled"), ui->tabContent->isAdBlockEnabled());
    m_settings->setValue(QStringLiteral("AutoLoadImages"), ui->tabContent->isAutoLoadImagesEnabled());
    m_settings->setValue(QStringLiteral("EnablePlugins"), ui->tabContent->arePluginsEnabled());
    m_settings->setValue(QStringLiteral("EnableJavascriptPopups"), ui->tabContent->arePopupsEnabled());
    m_settings->setValue(QStringLiteral("EnableJavascript"), ui->tabContent->isJavaScriptEnabled());
    m_settings->setValue(QStringLiteral("UserScriptsEnabled"), ui->tabContent->areUserScriptsEnabled());
    sBrowserApplication->getUserScriptManager()->setEnabled(ui->tabContent->areUserScriptsEnabled());

    // Save font choices, and also set them in the global web settings
    m_settings->setValue(QStringLiteral("StandardFont"), ui->tabContent->getDefaultFont());
    m_settings->setValue(QStringLiteral("SerifFont"), ui->tabContent->getSerifFont());
    m_settings->setValue(QStringLiteral("SansSerifFont"), ui->tabContent->getSansSerifFont());
    m_settings->setValue(QStringLiteral("CursiveFont"), ui->tabContent->getCursiveFont());
    m_settings->setValue(QStringLiteral("FantasyFont"), ui->tabContent->getFantasyFont());
    m_settings->setValue(QStringLiteral("FixedFont"), ui->tabContent->getFixedFont());

    m_settings->setValue(QStringLiteral("StandardFontSize"), ui->tabContent->getStandardFontSize());
    m_settings->setValue(QStringLiteral("FixedFontSize"), ui->tabContent->getFixedFontSize());

    sBrowserApplication->setWebSettings();

    close();
}

// Content tab items: Popup behavior (allow/deny), Browser fonts, Plugin support (such as flash) - Allow/deny
