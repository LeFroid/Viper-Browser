#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include <QString>

namespace AdBlock
{
    /**
     * @class AdBlockManager
     * @brief Manages a list of AdBlock Plus style subscriptions
     */
    class AdBlockManager
    {
    public:
        /// Constructs the AdBlockManager object
        AdBlockManager();

        /// Static AdBlockManager instance
        static AdBlockManager &instance();

    private:
        /// Loads active subscriptions
        void loadSubscriptions();

    private:
        /// True if AdBlock is enabled, false if disabled
        bool m_enabled;

        /// Directory path in which subscriptions are located
        QString m_subscriptionDir;
    };

}
#endif // ADBLOCKMANAGER_H
