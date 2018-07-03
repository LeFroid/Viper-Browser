#ifndef WEBWIDGET_H
#define WEBWIDGET_H

#include <QWidget>

class WebView;

class QIcon;
class QUrl;

/**
 * @class WebWidget
 * @brief A widget that is used as the container of a \ref WebView and
 *        acts as a bridge between the browser and the web engine implementation
 */
class WebWidget : public QWidget
{
    Q_OBJECT
public:
    /**
     * @brief Constructs the WebWidget
     * @param privateMode Set to true if the web view should be off-the-record, false if a regular web view
     * @param parent Pointer to the parent widget
     */
    explicit WebWidget(bool privateMode, QWidget *parent = nullptr);

    /// Returns the loading progress percentage value of the page as an integer in the range [0,100]
    int getProgress() const;

    /// Loads the resource of the blank page into the view
    void loadBlankPage();

    /// Returns true if the view's page is blank, with no resources being loaded
    bool isOnBlankPage() const;

    /// Returns the icon associated with the current page
    QIcon getIcon() const;

    /// Returns the URL of the icon associated with the current page
    QUrl getIconUrl() const;

    /**
     * @brief Returns the title of the web page being viewed
     *
     * Returns the title of the web page being viewed. If there is no title
     * for the page, and the path of the URL is not empty or the root path,
     * the path following the last '/' will be returned as the title. If only
     * the root path is available, the host will be returned as the title
     * @return The title of the web page being viewed
     */
    QString getTitle() const;

    /// Loads the specified url and displays it
    void load(const QUrl &url);

signals:
    /// Emitted when the icon associated with the view is changed. The new icon is specified by icon.
    void iconChanged(const QIcon &icon);

    /// Emitted when the URL of the icon associated with the view is changed. The new URL is specified by url.
    void iconUrlChanged(const QUrl &url);

    /*
public slots:*/

private:
    /// The actual web view widget being used to render and access web content
    WebView *m_view;
};

#endif // WEBWIDGET_H
