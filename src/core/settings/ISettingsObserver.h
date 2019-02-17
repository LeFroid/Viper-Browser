#ifndef ISETTINGSOBSERVER_H
#define ISETTINGSOBSERVER_H

#include "BrowserSetting.h"

#include <QVariant>

/**
 * @class ISettingsObserver
 * @brief Defines an interface for classes that need to subscribe
 *        to event notifications when there is a change to a browser
 *        setting
 */
class ISettingsObserver
{
private:
    /// Called when a browser setting has been changed to the given value
    virtual void onSettingChanged(BrowserSetting setting, const QVariant &value) = 0;
};

#endif // ISETTINGSOBSERVER_H
