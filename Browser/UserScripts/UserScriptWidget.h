#ifndef USERSCRIPTWIDGET_H
#define USERSCRIPTWIDGET_H

#include <QWidget>

namespace Ui {
    class UserScriptWidget;
}

/**
 * @class UserScriptWidget
 * @brief Acts as a management window for user scripts. The user can install remote scripts by URL,
 *        create new scripts locally, modify existing scripts, delete scripts from storage, and enable
 *        or disable existing scripts.
 */
class UserScriptWidget : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the management window. No parent widget should be given
    explicit UserScriptWidget(QWidget *parent = nullptr);

    /// Destroys the UI elements associated with the object
    ~UserScriptWidget();

private:
    /// Pointer to the user interface elements
    Ui::UserScriptWidget *ui;
};

#endif // USERSCRIPTWIDGET_H
