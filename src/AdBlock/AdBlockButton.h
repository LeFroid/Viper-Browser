#ifndef ADBLOCKBUTTON_H
#define ADBLOCKBUTTON_H

#include <QIcon>
#include <QTimer>
#include <QToolButton>

/**
 * @class AdBlockButton
 * @brief Acts as a visual indicator to the number of ads that are being blocked
 *        on a \ref WebPage. Displayed in the \ref NavigationToolBar
 */
class AdBlockButton : public QToolButton
{
    Q_OBJECT

public:
    /// Constructs the AdBlockButton with a given parent
    explicit AdBlockButton(QWidget *parent = nullptr);

public slots:
    /// Updates the region of the button's icon containing the number of ads being blocked on the active page
    void updateCount();

private:
    /// Base icon shown in the button
    QIcon m_icon;

    /// Timer that periodically calls the updateCount() method
    QTimer m_timer;

    /// The last reported number of ads that were blocked on a page when the updateCount() slot was called
    int m_lastCount;
};

#endif // ADBLOCKBUTTON_H
