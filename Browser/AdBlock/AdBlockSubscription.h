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
        AdBlockSubscription() = default;

    private:
        /// True if subscription is enabled, false if else
        bool m_enabled;

        /// Container of AdBlock Filters that belong to the subscription
        std::vector< std::unique_ptr<AdBlockFilter> > m_filters;
    };
}

#endif // ADBLOCKSUBSCRIPTION_H
