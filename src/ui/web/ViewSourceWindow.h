#ifndef VIEWSOURCEWINDOW_H
#define VIEWSOURCEWINDOW_H

#include <QString>
#include <QWidget>

namespace Ui {
    class ViewSourceWindow;
}

class HTMLHighlighter;
class WebPage;

/**
 * @class ViewSourceWindow
 * @brief Displays the HTML content of a \ref WebPage before any DOM manipulation may occur
 *        (other than by the WebPage itself).
 */
class ViewSourceWindow : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the window with a title and an optional parent
    explicit ViewSourceWindow(const QString &title, QWidget *parent = nullptr);

    /// Window destructor. Frees any resources allocated on the heap
    ~ViewSourceWindow();

    /// Sets the web page from which this widget extracts the source into its code editor view
    void setWebPage(WebPage *page);

private Q_SLOTS:
    /// Toggles the visibility of the find text widget group at the bottom of the window
    void toggleFindTextWidget();

private:
    /// Pointer to the UI elements in the corresponding .ui file
    Ui::ViewSourceWindow *ui;

    /// HTML syntax highlighter
    HTMLHighlighter *m_highlighter;
};

#endif // VIEWSOURCEWINDOW_H
