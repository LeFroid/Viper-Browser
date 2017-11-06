#ifndef CONTENTTAB_H
#define CONTENTTAB_H

#include <QWidget>

namespace Ui {
    class ContentTab;
}

/**
 * @class ContentTab
 * @brief Contains the web browser settings for general content, such as choice of fonts,
 *        whether or not to enable javascript, popups, etc.
 */
class ContentTab : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the content tab
    explicit ContentTab(QWidget *parent = 0);

    /// Destructor
    ~ContentTab();

    /// Returns true if plugins are enabled, false if disabled
    bool arePluginsEnabled() const;

    /// Returns true if popups are enabled, false if disabled
    bool arePopupsEnabled() const;

    /// Returns true if images should automatically be loaded, false if otherwise
    bool isAutoLoadImagesEnabled() const;

    /// Returns true if JavaScript is enabled, false if disabled
    bool isJavaScriptEnabled() const;

    /// Returns the browser's standard font
    QString getStandardFont() const;

    /// Sets the standard font of the web browser
    void setStandardFont(const QString &fontFamily);

    /// Returns the size of the web browser's standard font
    int getStandardFontSize() const;

    /// Sets the size of the browser's standard font
    void setStandardFontSize(int size);

public slots:
    /**
     * @brief toggleAutoLoadImages Toggles the setting for automatically loading images per page
     * @param value If true, images will be loaded automatically. If false, they will not be loaded automatically
     */
    void toggleAutoLoadImages(bool value);

    /**
     * @brief togglePlugins Toggles the setting for allowing plugins (ex: flash) to be loaded
     * @param value If true, plugins will be loaded. Otherwise, they will be disabled
     */
    void togglePlugins(bool value);

    /**
     * @brief togglePopupBlock Toggles the setting for allowing popup windows
     * @param value If true, popup windows will be blocked. If false, popups will be enabled
     */
    void togglePopupBlock(bool value);

    /**
     * @brief toggleJavaScript Toggles the setting for allowing JavaScript content
     * @param value If true, JavaScript will be enabled. If false, JavaScript will be disabled
     */
    void toggleJavaScript(bool value);

private:
    Ui::ContentTab *ui;
};

#endif // CONTENTTAB_H
