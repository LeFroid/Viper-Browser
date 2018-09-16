#include "AutoFillBridge.h"
#include "AutoFill.h"
#include "BrowserApplication.h"
#include "Settings.h"
#include "WebPage.h"

AutoFillBridge::AutoFillBridge(WebPage *parent) :
    QObject(parent),
    m_page(parent)
{
}

AutoFillBridge::~AutoFillBridge()
{
}

void AutoFillBridge::onFormSubmitted(const QString &/*pageUrl*/, const QString &/*username*/, const QString &/*password*/, const QByteArray &/*formData*/)
{
    if (!sBrowserApplication->getSettings()->getValue(BrowserSetting::EnableAutoFill).toBool())
        return;

    //sBrowserApplication->getAutoFill()->onFormSubmitted(m_page, pageUrl, username, password, formData);
}
