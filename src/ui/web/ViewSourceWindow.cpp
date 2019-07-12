#include "ViewSourceWindow.h"
#include "ui_ViewSourceWindow.h"

#include "HTMLHighlighter.h"
#include "TextEditorTextFinder.h"
#include "WebPage.h"

#include <QShortcut>
#include <QLineEdit>

ViewSourceWindow::ViewSourceWindow(const QString &title, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ViewSourceWindow),
    m_highlighter(new HTMLHighlighter)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);

    setWindowTitle(tr("Viewing Source of %1").arg(title));

    m_highlighter->setDocument(ui->sourceView->document());

    std::unique_ptr<ITextFinder> textFinder { std::make_unique<TextEditorTextFinder>() };
    static_cast<TextEditorTextFinder*>(textFinder.get())->setTextEdit(ui->sourceView);
    ui->widgetFindText->setTextFinder(std::move(textFinder));

    QShortcut *shortcutFind = new QShortcut(QKeySequence(tr("Ctrl+F")), this);
    connect(shortcutFind, &QShortcut::activated, this, &ViewSourceWindow::toggleFindTextWidget);
}

ViewSourceWindow::~ViewSourceWindow()
{
    delete ui;
}

void ViewSourceWindow::setWebPage(WebPage *page)
{
    page->toHtml([this](const QString &html) {
        ui->sourceView->setPlainText(html);
        ui->sourceView->setFocus();
    });
}

void ViewSourceWindow::toggleFindTextWidget()
{
    if (ui->widgetFindText->isHidden())
        ui->widgetFindText->show();

    QLineEdit *findTextEdit = ui->widgetFindText->getLineEdit();
    if (findTextEdit->hasFocus())
        ui->sourceView->setFocus();
    else
        findTextEdit->setFocus();
}
