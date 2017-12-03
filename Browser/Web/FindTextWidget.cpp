#include "FindTextWidget.h"
#include "ui_findtextwidget.h"
#include "WebView.h"

#include <QPaintEvent>
#include <QWebFrame>

FindTextWidget::FindTextWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FindTextWidget),
    m_view(nullptr),
    m_searchTerm(),
    m_numOccurrences(0),
    m_occurrenceCounter(0),
    m_highlightAll(false),
    m_matchCase(false)
{
    ui->setupUi(this);

    // Set focus policy of line edit
    ui->lineEdit->setFocusPolicy(Qt::StrongFocus);

    // Set icons
    ui->pushButtonPrev->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    ui->pushButtonNext->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    ui->pushButtonHide->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));

    // Connect signals to slots
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &FindTextWidget::onFindNext);
    connect(ui->pushButtonNext, &QPushButton::clicked, this, &FindTextWidget::onFindNext);
    connect(ui->pushButtonPrev, &QPushButton::clicked, this, &FindTextWidget::onFindPrev);
    connect(ui->pushButtonHighlightAll, &QPushButton::toggled, this, &FindTextWidget::onToggleHighlightAll);
    connect(ui->pushButtonMatchCase, &QPushButton::toggled, this, &FindTextWidget::onToggleMatchCase);
    connect(ui->pushButtonHide, &QPushButton::clicked, this, &FindTextWidget::hide);
}

FindTextWidget::~FindTextWidget()
{
    delete ui;
}

void FindTextWidget::setWebView(WebView *view)
{
    // Reset search term and "matches found" label
    ui->lineEdit->setText(QString());
    ui->labelMatches->setText(QString());
    m_searchTerm.clear();

    m_view = view;
}

QLineEdit *FindTextWidget::getLineEdit()
{
    return ui->lineEdit;
}

void FindTextWidget::onFindNext()
{
    if (!m_view)
        return;

    QFlags<QWebPage::FindFlag> flags = QWebPage::FindWrapsAroundDocument;
    if (m_highlightAll)
    {
        m_view->findText(QString(), QWebPage::HighlightAllOccurrences);
        flags |= QWebPage::HighlightAllOccurrences;
    }
    if (m_matchCase)
        flags |= QWebPage::FindCaseSensitively;

    if (m_view->findText(ui->lineEdit->text(), flags))
        setMatchCountLabel(true);
    else
    {
        if (m_searchTerm.isNull())
            ui->labelMatches->setText(QString());
        else
            ui->labelMatches->setText(QString("Phrase not found"));
    }
}

void FindTextWidget::onFindPrev()
{
    if (!m_view)
        return;

    QFlags<QWebPage::FindFlag> flags = QWebPage::FindBackward | QWebPage::FindWrapsAroundDocument;
    if (m_highlightAll)
    {
        m_view->findText(QString(), QWebPage::HighlightAllOccurrences);
        flags |= QWebPage::HighlightAllOccurrences;
    }
    if (m_matchCase)
        flags |= QWebPage::FindCaseSensitively;

    if (m_view->findText(ui->lineEdit->text(), flags))
        setMatchCountLabel(false);
    else
    {
        if (m_searchTerm.isNull())
            ui->labelMatches->setText(QString());
        else
            ui->labelMatches->setText(QString("Phrase not found"));
    }
}

void FindTextWidget::onToggleHighlightAll(bool checked)
{
    m_highlightAll = checked;
}

void FindTextWidget::onToggleMatchCase(bool checked)
{
    m_matchCase = checked;
}

void FindTextWidget::setMatchCountLabel(bool searchForNext)
{
    QString term = ui->lineEdit->text();
    if (term.isNull() || term.isEmpty())
    {
        m_searchTerm.clear();
        return;
    }

    // Check if we need to search the page again to find the number of occurrences
    if (m_searchTerm.compare(term) != 0)
    {
        m_searchTerm = term;
        m_occurrenceCounter = 0;

        QString plainText = m_view->page()->mainFrame()->toPlainText();
        Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;
        m_numOccurrences = plainText.count(m_searchTerm, caseSensitivity);
    }

    // Update position counter
    int positionModifier = searchForNext ? 1 : -1;
    m_occurrenceCounter = (m_occurrenceCounter + positionModifier) % (m_numOccurrences + 1);
    if (m_occurrenceCounter == 0)
        m_occurrenceCounter = searchForNext ? 1 : m_numOccurrences;

    // Update the label
    ui->labelMatches->setText(QString("%1 of %2 matches").arg(m_occurrenceCounter).arg(m_numOccurrences));
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

