#ifndef FINDTEXTWIDGET_H
#define FINDTEXTWIDGET_H

#include <QWidget>

namespace Ui {
class FindTextWidget;
}

class QLineEdit;
class ITextFinder;

/**
 * @class FindTextWidget
 * @brief Used to search for any given text in either a web page or a text document
 */
class FindTextWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the text finder widget with the given parent
    explicit FindTextWidget(QWidget *parent = nullptr);

    /// FindTextWidget destructor
    ~FindTextWidget();

    /// Sets the implementation that will be used to search for text.
    void setTextFinder(ITextFinder *textFinder);

    /// Returns a pointer to the text finder
    ITextFinder *getTextFinder() const;

    /// Returns a pointer to the line edit widget (used to acquire focus)
    QLineEdit *getLineEdit();

signals:
    /// Emitted when a text document is aesthetically modified by the text finder
    void pseudoModifiedDocument();

public slots:
    /// Clears the UI labels
    void clearLabels();

protected:
    /// Paints the widget onto its parent. This method is overrided in order to draw a border around the widget
    virtual void paintEvent(QPaintEvent *event) override;

private:
    /// Find text UI
    Ui::FindTextWidget *ui;

    /// Text finding implementation
    ITextFinder *m_textFinder;
};

#endif // FINDTEXTWIDGET_H
