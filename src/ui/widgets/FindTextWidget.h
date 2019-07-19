#ifndef FINDTEXTWIDGET_H
#define FINDTEXTWIDGET_H

#include <memory>
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

    /// Sets the instance of the text finder that will be used by this widget.
    void setTextFinder(std::unique_ptr<ITextFinder> &&textFinder);

    /// Returns a pointer to the text finder
    ITextFinder *getTextFinder() const;

    /// Returns a pointer to the line edit widget (used to acquire focus)
    QLineEdit *getLineEdit();

Q_SIGNALS:
    /// Emitted when a text document is aesthetically modified by the text finder
    void pseudoModifiedDocument();

public Q_SLOTS:
    /// Clears the UI labels
    void clearLabels();

protected:
    /// Paints the widget onto its parent. This method is overrided in order to draw a border around the widget
    virtual void paintEvent(QPaintEvent *event) override;

private:
    /// Find text UI
    Ui::FindTextWidget *ui;

    /// Text finding implementation
    std::unique_ptr<ITextFinder> m_textFinder;
};

#endif // FINDTEXTWIDGET_H
