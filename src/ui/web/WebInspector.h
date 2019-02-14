#ifndef WEBINSPECTOR_H
#define WEBINSPECTOR_H

#include "WebView.h"

/**
 * @class WebInspector
 * @brief Acts as the inspector to a specific \ref WebView .
 */
class WebInspector : public WebView
{
    Q_OBJECT

public:
    /// Constructs the web inspector. If privateView is true, this inspector is associated
    /// with a private browsing profile
    explicit WebInspector(bool privateView, QWidget* parent = nullptr);

    /// Returns true if the inspector is visible alongside its web view, returns false otherwise
    bool isActive() const;

    /// Sets the activity state of the inspector
    void setActive(bool value);

private:
    /// Flag indicating whether or not the inspector is active or visible
    bool m_active;
};

#endif // WEBINSPECTOR_H
