#include "welcome_window/WelcomeWindow.h"

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include <QDebug>

WelcomeWindow::WelcomeWindow(QWidget *parent) :
    QWizard(parent)
{
    setWindowTitle(tr("Profile Setup"));

    addPage(new IntroductionPage);
}

void WelcomeWindow::accept()
{
    qDebug() << "WelcomeWindow::accept()";
}

IntroductionPage::IntroductionPage(QWidget *parent) :
    QWizardPage(parent),
    m_labelPageDescription(nullptr)
{
    setTitle(tr("Introduction"));

    m_labelPageDescription
            = new QLabel(tr("Thank you for installing Viper Browser! The following pages will "
                            "ask you to set your profile settings for an optimal browsing experience. "
                            "This includes choice of search engine, home page, advertisement blocking "
                            "subscriptions (\"filter lists\"), and whether or not plugins like Flash "
                            "should be enabled."));
    m_labelPageDescription->setWordWrap(true);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->addWidget(m_labelPageDescription);
    setLayout(vboxLayout);
}
