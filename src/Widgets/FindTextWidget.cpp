#include "FindTextWidget.h"
#include "ui_FindTextWidget.h"
#include "ITextFinder.h"
#include "WebView.h"

#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QStyle>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>

FindTextWidget::FindTextWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FindTextWidget),
    m_textFinder(nullptr)
{
    ui->setupUi(this);

    // Set focus policy of line edit
    ui->lineEdit->setFocusPolicy(Qt::StrongFocus);

    // Set icons
    ui->pushButtonPrev->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    ui->pushButtonNext->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    ui->pushButtonHide->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
}

FindTextWidget::~FindTextWidget()
{
    if (m_textFinder != nullptr)
    {
        delete m_textFinder;
        m_textFinder = nullptr;
    }

    delete ui;
}

void FindTextWidget::setTextFinder(ITextFinder *textFinder)
{
    if (!textFinder)
        return;

    if (m_textFinder != nullptr)
        delete m_textFinder;

    clearLabels();

    m_textFinder = textFinder;

    connect(m_textFinder, &ITextFinder::showMatchResultText,   ui->labelMatches, &QLabel::setText);
    connect(m_textFinder, &ITextFinder::pseudoModifiedDocument, this, &FindTextWidget::pseudoModifiedDocument);

    connect(ui->lineEdit,               &QLineEdit::textChanged,   m_textFinder, &ITextFinder::searchTermChanged);
    connect(ui->lineEdit,               &QLineEdit::returnPressed, m_textFinder, &ITextFinder::findNext);
    connect(ui->pushButtonNext,         &QPushButton::clicked,     m_textFinder, &ITextFinder::findNext);
    connect(ui->pushButtonPrev,         &QPushButton::clicked,     m_textFinder, &ITextFinder::findPrevious);
    connect(ui->pushButtonHighlightAll, &QPushButton::toggled,     m_textFinder, &ITextFinder::setHighlightAll);
    connect(ui->pushButtonMatchCase,    &QPushButton::toggled,     m_textFinder, &ITextFinder::setMatchCase);

    connect(ui->pushButtonHide, &QPushButton::clicked, [this](){
        if (m_textFinder)
            m_textFinder->stopSearching();

        hide();
    });
}

ITextFinder *FindTextWidget::getTextFinder() const
{
    return m_textFinder;
}

void FindTextWidget::clearLabels()
{
    ui->lineEdit->setText(QString());
    ui->labelMatches->setText(QString());
}

QLineEdit *FindTextWidget::getLineEdit()
{
    return ui->lineEdit;
}

void FindTextWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QColor(102, 102, 102));
    pen.setWidth(2);
    painter.drawLine(0, 0, width(), 0);

    QWidget::paintEvent(event);
}

