#ifndef URLSUGGESTIONWIDGET_H
#define URLSUGGESTIONWIDGET_H

#include "ServiceLocator.h"

#include <QString>
#include <QThread>
#include <QWidget>

class URLLineEdit;
class URLSuggestionListModel;
class URLSuggestionWorker;

class QListView;
class QModelIndex;

/**
 * @class URLSuggestionWidget
 * @brief Acts as a container for suggesting and displaying URLs based on user input in the \ref URLLineEdit
 */
class URLSuggestionWidget final : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the URL suggestion widget
    explicit URLSuggestionWidget(QWidget *parent = nullptr);

    /// Destructor
    ~URLSuggestionWidget();

    /// Filters events if this object has been installed as an event filter for the watched object.
    bool eventFilter(QObject *watched, QEvent *event) override;

    /// Aligns the suggestion widget, based on the position and size of the URL line edit, and shows the widget
    void alignAndShow(const QPoint &urlBarPos, const QRect &urlBarGeom);

    /// Uses the given input string to search for URLs to suggest to the user
    void suggestForInput(const QString &text);

    /// Sets the pointer to the URL line edit
    void setURLLineEdit(URLLineEdit *lineEdit);

    /// Sets the reference to the service locator, which is passed on to the \ref URLSuggestionWorker
    void setServiceLocator(const ViperServiceLocator &serviceLocator);

    /// Returns the suggested size of the widget
    QSize sizeHint() const override;

    /// Called by the URL line edit when its own size has changed, sets the minimum width of this widget to that of the URL widget
    void needResizeWidth(int width);

Q_SIGNALS:
    /// Emitted when an item in the suggestion list with the given URL is chosen
    void urlChosen(const QUrl &url);

    /**
     * @brief Emitted when the widget is hidden before the user chooses any of the suggestions.
     * @param originalText The text entered by the user before this widget appeared
     */
    void noSuggestionChosen(const QString &originalText);

    /// Bound to the suggestion worker's findSuggestionsFor(...) slot
    void determineSuggestions(const QString &text);

private Q_SLOTS:
    /// Called when an item in the suggestion list at the given index is clicked
    void onSuggestionClicked(const QModelIndex &index);

private:
    /// List view containing suggested URLs
    QListView *m_suggestionList;

    /// Model containing suggestions listed in the list view
    URLSuggestionListModel *m_model;

    /// Worker that fetches the suggestions
    URLSuggestionWorker *m_worker;

    /// Pointer to the URL line edit
    URLLineEdit *m_lineEdit;

    /// The term being searched for suggestions
    QString m_searchTerm;

    /// Worker thread
    QThread m_workerThread;
};

#endif // URLSUGGESTIONWIDGET_H
