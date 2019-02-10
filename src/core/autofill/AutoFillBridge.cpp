#include "AutoFillBridge.h"
#include "AutoFill.h"
#include "BrowserApplication.h"
#include "Settings.h"
#include "WebPage.h"

#include <QDebug>

AutoFillBridge::AutoFillBridge(AutoFill *autoFill, WebPage *parent) :
    QObject(parent),
    m_page(parent),
    m_autoFill(autoFill)
{
}

AutoFillBridge::~AutoFillBridge()
{
}

void AutoFillBridge::onFormSubmitted(const QString &pageUrl, const QString &username, const QString &password, const QMap<QString, QVariant> &formData)
{
    m_autoFill->onFormSubmitted(m_page, pageUrl, username, password, formData);
}
