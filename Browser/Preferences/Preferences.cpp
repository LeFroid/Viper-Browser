#include "Preferences.h"
#include "ui_Preferences.h"

#include "Settings.h"
#include <QDir>
#include <QWebSettings>

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
    ui->tabGeneral->setDownloadDirectory(m_settings->getValue("DownloadDir").toString());
    ui->tabGeneral->setDownloadAskBehavior(m_settings->getValue("AskWhereToSaveDownloads").toBool());
    ui->tabGeneral->setHomePage(m_settings->getValue("HomePage").toString());
    ui->tabGeneral->setStartupIndex(m_settings->getValue("StartupMode").toInt());
    ui->tabGeneral->setNewTabsLoadHomePage(m_settings->getValue("NewTabsLoadHomePage").toBool());

    ui->tabContent->toggleAutoLoadImages(m_settings->getValue("AutoLoadImages").toBool());
    ui->tabContent->togglePopupBlock(!m_settings->getValue("EnableJavascriptPopups").toBool());
    ui->tabContent->toggleJavaScript(m_settings->getValue("EnableJavascript").toBool());
    ui->tabContent->setStandardFont(m_settings->getValue("StandardFont").toString());
    ui->tabContent->setStandardFontSize(m_settings->getValue("StandardFontSize").toInt());
}

void Preferences::onCloseWithSave()
{
    if (!m_settings)
        return;

    // Fetch preferences in the General tab
    QString currItem = ui->tabGeneral->getHomePage();
    if (!currItem.isEmpty())
        m_settings->setValue("HomePage", currItem);

    currItem = ui->tabGeneral->getDownloadDirectory();
    QDir downDir(currItem);
    if (downDir.exists())
        m_settings->setValue("DownloadDir", currItem);

    m_settings->setValue("AskWhereToSaveDownloads", ui->tabGeneral->getDownloadAskBehavior());

    m_settings->setValue("StartupMode", ui->tabGeneral->getStartupIndex());
    m_settings->setValue("NewTabsLoadHomePage", ui->tabGeneral->doNewTabsLoadHomePage());

    // Fetch preferences in Content tab
    m_settings->setValue("AutoLoadImages", ui->tabContent->isAutoLoadImagesEnabled());
    m_settings->setValue("EnableJavascriptPopups", ui->tabContent->arePopupsEnabled());
    m_settings->setValue("EnableJavascript", ui->tabContent->isJavaScriptEnabled());

    // Save font choices, and also set them in the global web settings
    QWebSettings *webSettings = QWebSettings::globalSettings();
    QString standardFont = ui->tabContent->getStandardFont();
    int standardFontSize = ui->tabContent->getStandardFontSize();

    m_settings->setValue("StandardFont", standardFont);
    m_settings->setValue("StandardFontSize", standardFontSize);

    webSettings->setFontFamily(QWebSettings::StandardFont, standardFont);
    webSettings->setFontSize(QWebSettings::DefaultFontSize, standardFontSize);

    close();
}

// Content tab items: Popup behavior (allow/deny), Browser fonts, Plugin support (such as flash) - Allow/deny
