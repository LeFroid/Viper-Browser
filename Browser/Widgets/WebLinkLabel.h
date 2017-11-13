#ifndef WEBLINKLABEL_H
#define WEBLINKLABEL_H

#include <QLabel>

class WebPage;

/**
 * @class WebLinkLabel
 * @brief The label shown in the viewport when the user hovers over a link on a \ref WebPage
 */
class WebLinkLabel : public QLabel
{
    Q_OBJECT
public:
    /**
     * @brief WebLinkLabel Constructs the web link label with the given parent
     * @param parent Pointer to the \ref WebView that will own this object
     * @param page Pointer to the web page, owned by the \ref WebView , that this link label will
     *             use to bind link hover events to the displaying of the URL in the label content area
     */
    explicit WebLinkLabel(QWidget *parent, WebPage *page);

protected:
    /// Returns the recommended size of the label, based on the width of the link currently being shown
    QSize sizeHint() const override;

private slots:
    /// Called when the user hovers over a link, in order to display its location
    void showLinkRef(const QString &link, const QString &title, const QString &context);

private:
    /// Flag set to true when showing a link. Used for edge cases where label is not hidden between
    /// two different links being hovered, and the size would otherwise be miscalculated
    bool m_showingLink;
};

#endif // WEBLINKLABEL_H
