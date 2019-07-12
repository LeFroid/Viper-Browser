#include "FindTextWidget.h"
#include "ui_FindTextWidget.h"
#include "ITextFinder.h"

#include <QIcon>
#include <QPainter>
#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QString>
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
    ui->pushButtonHide->setIcon(QIcon(QLatin1String(":/window-close.png")));
    //ui->pushButtonHide->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
}

FindTextWidget::~FindTextWidget()
{
    delete ui;
}

void FindTextWidget::setTextFinder(std::unique_ptr<ITextFinder> &&textFinder)
{
    if (!textFinder.get())
        return;

    m_textFinder.reset(nullptr);
    m_textFinder = std::move(textFinder);

    clearLabels();

    auto textFinderPtr = m_textFinder.get();
    connect(textFinderPtr, &ITextFinder::showMatchResultText,   ui->labelMatches, &QLabel::setText);
    connect(textFinderPtr, &ITextFinder::pseudoModifiedDocument, this, &FindTextWidget::pseudoModifiedDocument);

    connect(ui->lineEdit,               &QLineEdit::textChanged,   textFinderPtr, &ITextFinder::searchTermChanged);
    connect(ui->lineEdit,               &QLineEdit::returnPressed, textFinderPtr, &ITextFinder::findNext);
    connect(ui->pushButtonNext,         &QPushButton::clicked,     textFinderPtr, &ITextFinder::findNext);
    connect(ui->pushButtonPrev,         &QPushButton::clicked,     textFinderPtr, &ITextFinder::findPrevious);
    connect(ui->pushButtonHighlightAll, &QPushButton::toggled,     textFinderPtr, &ITextFinder::setHighlightAll);
    connect(ui->pushButtonMatchCase,    &QPushButton::toggled,     textFinderPtr, &ITextFinder::setMatchCase);

    connect(ui->pushButtonHide, &QPushButton::clicked, [this](){
        if (m_textFinder)
            m_textFinder->stopSearching();

        hide();
    });
}

ITextFinder *FindTextWidget::getTextFinder() const
{
    return m_textFinder.get();
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

