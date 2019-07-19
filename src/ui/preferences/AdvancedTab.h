#ifndef ADVANCEDTAB_H
#define ADVANCEDTAB_H

#include <QString>
#include <QWidget>

namespace Ui {
    class AdvancedTab;
}

/**
 * @class AdvancedTab
 * @brief Represents the more advanced configuration settings that are exposed to the user.
 *        This tab is shown in the \ref Preferences widget.
 */
class AdvancedTab : public QWidget
{
    Q_OBJECT

public:
    /// Constructs the widget
    explicit AdvancedTab(QWidget *parent = nullptr);

    /// Destructor
    ~AdvancedTab();

    /// Sets the process model flag to be shown in the combo box
    void setProcessModel(const QString &value);

    /// Returns the current process model flag
    QString getProcessModel() const;

    /// Sets the web engine GPU flag (disabled if value == true, otherwise enabled)
    void setGpuDisabled(bool value);

    /// Returns true if the GPU should be disabled, false if else
    bool isGpuDisabled() const;

private Q_SLOTS:
    /// Changes the text next to the checkbox depending on the given value
    void onGpuCheckBoxChanged(bool value);

private:
    /// Pointer to the UI elements
    Ui::AdvancedTab *ui;
};

#endif // ADVANCEDTAB_H

