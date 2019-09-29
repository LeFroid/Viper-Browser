#ifndef WELCOMEWINDOW_H
#define WELCOMEWINDOW_H

#include <QWizard>

class QLabel;

/**
 * @class WelcomeWindow
 * @brief This class walks the user through some initial profile
 *        setup questions, including choice of ad-block subscriptions,
 *        a home page, default search engine, etc.
 */
class WelcomeWindow : public QWizard
{
    Q_OBJECT

public:
    /// Constructs the welcome window, with an optional parent
    WelcomeWindow(QWidget *parent = nullptr);

    /// Invoked when the dialog is accepted or submitted by the user
    void accept() override;
};

/**
 * @class IntroductionPage
 * @brief This is the first page of the welcome window. It explains
 *        the purpose of the dialog and what inputs the user will
 *        be asked to provide in the subsequent pages.
 */
class IntroductionPage : public QWizardPage
{
    Q_OBJECT

public:
    /// Constructs the introduction page
    IntroductionPage(QWidget *parent = nullptr);

private:
    /// Contains the majority of text on the page
    QLabel *m_labelPageDescription;
};

#endif // WELCOMEWINDOW_H
