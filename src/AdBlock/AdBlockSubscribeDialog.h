#ifndef ADBLOCKSUBSCRIBEDIALOG_H
#define ADBLOCKSUBSCRIBEDIALOG_H

#include <QDialog>
#include <QString>
#include <QUrl>

#include <vector>

namespace Ui {
    class AdBlockSubscribeDialog;
}

/**
 * @ingroup AdBlock
 * @brief Stores information needed to install one of the dialog's recommended subscriptions
 */
struct AdBlockSubscriptionInfo
{
    /// Name of subscription
    QString Name;

    /// URL of the subscription file
    QUrl SubscriptionURL;

    /// URL of the resource file associated with subscription (if any)
    QUrl ResourceURL;
};

/**
 * @class AdBlockSubscribeDialog
 * @ingroup AdBlock
 * @brief Displays a list of featured subscriptions which the user can check
 *        on in order to add them to their profile
 */
class AdBlockSubscribeDialog : public QDialog
{
    Q_OBJECT

public:
    /// Constructs the subscription dialog
    explicit AdBlockSubscribeDialog(QWidget *parent = 0);

    /// Dialog destructor
    ~AdBlockSubscribeDialog();

    /// Returns a container of subscriptions the user has selected to be installed
    std::vector<AdBlockSubscriptionInfo> getSubscriptions() const;

private:
    /// Loads recommended subscriptions into the list widget
    void load();

private:
    /// Pointer to the user interface elements
    Ui::AdBlockSubscribeDialog *ui;

    /// Container of recommended subscriptions
    std::vector<AdBlockSubscriptionInfo> m_subscriptions;
};

#endif // ADBLOCKSUBSCRIBEDIALOG_H
