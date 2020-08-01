#include "CommonUtil.h"
#include "HistoryManager.h"
#include "WebLoadObserver.h"
#include "WebWidget.h"

#include <array>
#include <QUrl>
#include <QString>

WebLoadObserver::WebLoadObserver(HistoryManager *historyManager, WebWidget *parent) :
    QObject(parent),
    m_historyManager(historyManager),
    m_webWidget(parent)
{
    connect(parent, &WebWidget::loadFinished, this, &WebLoadObserver::onLoadFinished);
}

void WebLoadObserver::onLoadFinished(bool ok)
{
    if (!ok)
        return;

    const QString title = m_webWidget->getTitle();
    const QUrl url = m_webWidget->url();
    const QUrl originalUrl = m_webWidget->getOriginalUrl();
    const QString scheme = url.scheme().toLower();

    // Check if we should ignore this page
    const std::array<QString, 2> ignoreSchemes { QStringLiteral("qrc"), QStringLiteral("viper") };
    if (url.isEmpty() || originalUrl.isEmpty() || scheme.isEmpty())
        return;

    for (const QString &ignoreScheme : ignoreSchemes)
    {
        if (scheme == ignoreScheme)
            return;
    }

    const QUrl &lastTypedUrl = m_webWidget->getLastTypedUrl();
    const bool wasTypedByUser = !lastTypedUrl.isEmpty()
            && (CommonUtil::doUrlsMatch(originalUrl, lastTypedUrl, true) || CommonUtil::doUrlsMatch(url, lastTypedUrl, true));
    if (wasTypedByUser)
        m_webWidget->clearLastTypedUrl();

    // Notify the history manager
    m_historyManager->addVisit(url, title, QDateTime::currentDateTime(), originalUrl, wasTypedByUser);
}
