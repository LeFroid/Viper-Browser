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
    explicit ContentTab(QWidget *parent = nullptr);

    /// Destructor
    ~ContentTab();

    /// Returns true if plugins are enabled, false if disabled
    bool arePluginsEnabled() const;

    /// Returns true if popups are enabled, false if disabled
    bool arePopupsEnabled() const;

    /// Returns true if the user script system is enabled, false if disabled
    bool areUserScriptsEnabled() const;

    /// Returns true if advertisement blocking is enabled, false if disabled
    bool isAdBlockEnabled() const;

    /// Returns true if animated scrolling is enabled, false if disabled
    bool isAnimatedScrollEnabled() const;

    /// Returns true if images should automatically be loaded, false if otherwise
    bool isAutoLoadImagesEnabled() const;

    /// Returns true if JavaScript is enabled, false if disabled
    bool isJavaScriptEnabled() const;

    /// Returns the browser's default font family
    QString getDefaultFont() const;

    /// Sets the default font family of the web browser
    void setDefaultFont(const QString &fontFamily);

    /// Returns the browser's serif font family
    QString getSerifFont() const;

    /// Sets the web browser's serif font family
    void setSerifFont(const QString &fontFamily);

    /// Returns the browser's sans-serif font family
    QString getSansSerifFont() const;

    /// Sets the web browser's sans-serif font family
    void setSansSerifFont(const QString &fontFamily);

    /// Returns the browser's cursive font family
    QString getCursiveFont() const;

    /// Sets the web browser's serif font family
    void setCursiveFont(const QString &fontFamily);

    /// Returns the browser's fantasy font family
    QString getFantasyFont() const;

    /// Sets the web browser's serif font family
    void setFantasyFont(const QString &fontFamily);

    /// Returns the browser's fixed font family
    QString getFixedFont() const;

    /// Sets the web browser's serif font family
    void setFixedFont(const QString &fontFamily);

    /// Returns the size of the web browser's standard font
    int getStandardFontSize() const;

    /// Sets the size of the browser's standard font
    void setStandardFontSize(int size);

    /// Returns the size of the browser's fixed font
    int getFixedFontSize() const;

    /// Sets the size of the browser's fixed font
    void setFixedFontSize(int size);

public Q_SLOTS:
    /**
     * @brief toggleAdBlock Toggles the setting for using an advertisement blocker when loading content
     * @param value If true, the ad blocker will be enabled. Otherwise it will be disabled
     */
    void toggleAdBlock(bool value);

    /**
     * @brief toggleAnimatedScrolling Toggles the setting for allowing animated scrolling of web content.
     * @param value If true, animated scrolling will be enabled, otherwise will be disabled.
     */
    void toggleAnimatedScrolling(bool value);

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

    /**
     * @brief toggleUserScripts Toggles the setting for allowing user scripts to be injected into web pages
     * @param value If true, user scripts will be enabled. If false, they will be disabled
     */
    void toggleUserScripts(bool value);

private:
    Ui::ContentTab *ui;
};

#endif // CONTENTTAB_H
