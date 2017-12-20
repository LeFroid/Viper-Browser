#include "AdBlockSubscription.h"

namespace AdBlock
{
    AdBlockSubscription::AdBlockSubscription() :
        m_enabled(true)
    {
    }

    bool AdBlockSubscription::isEnabled() const
    {
        return m_enabled;
    }

    void AdBlockSubscription::setEnabled(bool value)
    {
        m_enabled = value;
    }
}
