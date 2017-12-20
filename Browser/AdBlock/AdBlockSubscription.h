#ifndef ADBLOCKSUBSCRIPTION_H
#define ADBLOCKSUBSCRIPTION_H

#include <memory>
#include <vector>

namespace AdBlock
{
    class AdBlockFilter;

    /**
     * @class AdBlockSubscription
     * @brief Manages a list of ad block filters
     */
    class AdBlockSubscription
    {
    public:
        /// Constructs the AdBlockSubscription object
        AdBlockSubscription();

        /// Returns true if the subscription is enabled, false if disabled
        bool isEnabled() const;

        /**
         * @brief Toggles the state of the subscription
         * @param value If true, enables the subscription. If false, disables the subscription
         */
        void setEnabled(bool value);

    private:
        /// True if subscription is enabled, false if else
        bool m_enabled;

        /// Container of AdBlock Filters that belong to the subscription
        //std::vector<AdBlockFilter> m_filters;
    };
}

#endif // ADBLOCKSUBSCRIPTION_H
