#ifndef URLSUGGESTIONWIDGET_H
#define URLSUGGESTIONWIDGET_H

#include <QString>
#include <QWidget>

class URLLineEdit;
class URLSuggestionListModel;
class URLSuggestionWorker;
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

    /// Filters events if this object has been installed as an event filter for the watched object.
    bool eventFilter(QObject *watched, QEvent *event) override;

    /// Aligns the suggestion widget, based on the position and size of the URL line edit, and shows the widget
    void alignAndShow(const QPoint &urlBarPos, const QRect &urlBarGeom);

    /// Uses the given input string to search for URLs to suggest to the user
    void suggestForInput(const QString &text);

    /// Sets the pointer to the URL line edit
    void setURLLineEdit(URLLineEdit *lineEdit);

    /// Returns the suggested size of the widget
    QSize sizeHint() const override;

    /// Called by the URL line edit when its own size has changed, sets the minimum width of this widget to that of the URL widget
    void needResizeWidth(int width);

signals:
    void urlChosen(const QUrl &url);

private:
    /// List view containing suggested URLs
    QListView *m_suggestionList;

    /// Model containing suggestions listed in the list view
    URLSuggestionListModel *m_model;

    /// Worker that fetches the suggestions
    URLSuggestionWorker *m_worker;

    /// Pointer to the URL line edit
    URLLineEdit *m_lineEdit;
};

#endif // URLSUGGESTIONWIDGET_H
