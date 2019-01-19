#ifndef BROWSERSCRIPTS_H
#define BROWSERSCRIPTS_H

#include <vector>
#include <QWebEngineScript>

/**
 * @class BrowserScripts
 * @brief Contains various scripts (JavaScript) that extend
 *        web browser functionality and/or act as a bridge between
 *        the web engine and the core components of the application.
 */
class BrowserScripts
{
public:
    /// Constructs the browser script container class
    BrowserScripts();

    /// Returns a reference to the global scripts that are applied to both public and private browsing profiles
    const std::vector<QWebEngineScript> &getGlobalScripts() const;

    /// Returns a reference to the scripts that are applied to only the public browsing profile
    const std::vector<QWebEngineScript> &getPublicOnlyScripts() const;

private:
    /// Initializes the script that listens for form submission events, in order to be sent to the \ref AutoFill handler
    void initAutoFillObserverScript();

    /// Initializes the script that sends favicon data to the \ref FaviconStore
    void initFaviconScript();

    /// Initializes the script that is used to populate the "New Tab" page (different from blank page, this shows a list of
    /// favorite websites)
    void initNewTabScript();

    /// Initializes the script that implements the 'window.print' function
    void initPrintScript();

    /// Initializes the script that injects the web channel into each page
    void initWebChannelScript();

private:
    /// Contains scripts that are common to public and private browsing profiles
    std::vector<QWebEngineScript> m_globalScripts;

    /// Contains scripts that are only applied to the public browsing profile
    std::vector<QWebEngineScript> m_publicOnlyScripts;
};

#endif  // BROWSERSCRIPTS_H
