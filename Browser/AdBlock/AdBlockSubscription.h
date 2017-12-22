#ifndef ADBLOCKSUBSCRIPTION_H
#define ADBLOCKSUBSCRIPTION_H

#include "AdBlockFilter.h"
#include <memory>
#include <vector>
#include <QString>

namespace AdBlock
{
    /**
     * @class AdBlockSubscription
     * @brief Manages a list of ad block filters
     */
    class AdBlockSubscription
    {
        friend class AdBlockManager;

    public:
        /// Constructs the AdBlockSubscription object
        AdBlockSubscription();

        /// Constructs the AdBlockSubscription with the path to its data file
        AdBlockSubscription(const QString &dataFile);

        /// Returns true if the subscription is enabled, false if disabled
        bool isEnabled() const;

        /**
         * @brief Toggles the state of the subscription
         * @param value If true, enables the subscription. If false, disables the subscription
         */
        void setEnabled(bool value);

    protected:
        /// Loads the filters from the subscription file
        void load();

        /// Returns the number of filters that belong to the subscription
        int getNumFilters() const;

        /// Returns the filter at the given index
        AdBlockFilter *getFilter(int index);

    private:
        /// True if subscription is enabled, false if else
        bool m_enabled;

        /// Path to the subscription file
        QString m_filePath;

        /// Container of AdBlock Filters that belong to the subscription
        std::vector< std::unique_ptr<AdBlockFilter> > m_filters;
    };
}

#endif // ADBLOCKSUBSCRIPTION_H
