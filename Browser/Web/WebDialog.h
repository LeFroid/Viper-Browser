#ifndef WEBDIALOG_H
#define WEBDIALOG_H

#include <QWidget>

class QWebView;
class WebView;

/**
 * @class WebDialog
 * @brief Widget used to display popup windows
 */
class WebDialog : public QWidget
{
    Q_OBJECT
public:
    explicit WebDialog(QWidget *parent = nullptr);

    /// Returns the view associated with the dialog
    QWebView *getView() const;

/*
public slots:
*/

private:
    /// Web view associated with the dialog
    WebView *m_view;
};

#endif // WEBDIALOG_H
