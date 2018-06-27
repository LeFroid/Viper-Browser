#ifndef URLSUGGESTIONWIDGET_H
#define URLSUGGESTIONWIDGET_H

#include <QWidget>

class URLLineEdit;
class QListView;

/**
 * @class URLSuggestionWidget
 * @brief Acts as a container for suggesting and displaying URLs based on user input in the \ref URLLineEdit
 */
class URLSuggestionWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the URL suggestion widget
    explicit URLSuggestionWidget(QWidget *parent = nullptr);

    /// Aligns the suggestion widget, based on the position and size of the URL line edit, and shows the widget
    void alignAndShow(const QPoint &urlBarPos, const QRect &urlBarGeom);

    /// Sets the pointer to the URL line edit
    void setURLLineEdit(URLLineEdit *lineEdit);

    /// Returns the suggested size of the widget
    QSize sizeHint() const override;

    /// Called by the URL line edit when its own size has changed, sets the minimum width of this widget to that of the URL widget
    void needResizeWidth(int width);

private:
    /// List view containing suggested URLs
    QListView *m_suggestionList;

    /// Pointer to the URL line edit
    URLLineEdit *m_lineEdit;
};

#endif // URLSUGGESTIONWIDGET_H
