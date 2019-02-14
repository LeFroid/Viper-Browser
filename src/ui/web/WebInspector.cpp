#include "WebInspector.h"

WebInspector::WebInspector(bool privateView, QWidget* parent) :
    WebView(privateView, parent),
    m_active(false)
{
    setObjectName(QLatin1String("inspectorView"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContextMenuPolicy(Qt::NoContextMenu);
}

bool WebInspector::isActive() const
{
    return m_active;
}

void WebInspector::setActive(bool value)
{
    m_active = value;
}
