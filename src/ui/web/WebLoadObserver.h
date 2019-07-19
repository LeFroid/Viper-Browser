#ifndef WEBLOADOBSERVER_H
#define WEBLOADOBSERVER_H

#include <QObject>

class HistoryManager;
class WebWidget;

/**
 * @class WebLoadObserver
 * @brief Observes a \ref WebWidget , waiting for a loadFinished() signal so it may
 *        notify the \ref HistoryManager of such an event.
 */
class WebLoadObserver : public QObject
{
    Q_OBJECT

public:
    /// Constructs the load observer given a pointer to the history manager and the observer's parent
    explicit WebLoadObserver(HistoryManager *historyManager, WebWidget *parent);

private Q_SLOTS:
    /// Handles the load finished event - notifying the history manager if the load was a success
    void onLoadFinished(bool ok);

private:
    /// Pointer to the application's history manager
    HistoryManager *m_historyManager;

    /// Pointer to the web widget that this object is observing
    WebWidget *m_webWidget;
};

#endif // WEBLOADOBSERVER_H
