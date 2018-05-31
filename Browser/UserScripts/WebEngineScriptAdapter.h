#ifndef WEBENGINESCRIPTADAPTER_H
#define WEBENGINESCRIPTADAPTER_H

#include "UserScript.h"

#include <QWebEngineScript>

/**
 * @class WebEngineScriptAdapter
 * @brief Handles conversion between UserScript API and QWebEngineScript system
 */
class WebEngineScriptAdapter
{
public:
    /// Constructs the WebEngineScriptAdapter given a reference to an existing user script
    explicit WebEngineScriptAdapter(const UserScript &script);

    /// Returns a QWebEngineScript that represents the data contained in a \ref UserScript
    const QWebEngineScript &getScript();

private:
    /// The QWebEngineScript representing the original \ref UserScript passed to the adapter
    QWebEngineScript m_script;
};

#endif // WEBENGINESCRIPTADAPTER_H
