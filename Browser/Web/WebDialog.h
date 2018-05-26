#ifndef WEBDIALOG_H
#define WEBDIALOG_H

#include <QWidget>

class QWebEngineView;
class WebView;

/**
 * @class WebDialog
 * @brief Widget used to display popup windows
 */
class WebDialog : public QWidget
{
    Q_OBJECT
public:
    explicit WebDialog(bool isPrivate, QWidget *parent = nullptr);

    /// Returns the view associated with the dialog
    QWebEngineView *getView() const;

private:
    /// Web view associated with the dialog
    WebView *m_view;
};

#endif // WEBDIALOG_H
