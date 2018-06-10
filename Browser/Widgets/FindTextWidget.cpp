#include "FindTextWidget.h"
#include "ui_FindTextWidget.h"
#include "WebView.h"

#include <QPaintEvent>
#include <QPlainTextEdit>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
//#include <QWebFrame>

FindTextWidget::FindTextWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FindTextWidget),
    m_editor(nullptr),
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
    connect(ui->pushButtonHide, &QPushButton::clicked, [=](){
        if (m_editor != nullptr)
            resetEditorChanges();
        else if (m_view != nullptr)
            m_view->findText(QString());

        hide();
    });
}

FindTextWidget::~FindTextWidget()
{
    delete ui;
}

void FindTextWidget::setTextEdit(QPlainTextEdit *editor)
{
    // Reset search term and "matches found" label
    ui->lineEdit->setText(QString());
    ui->labelMatches->setText(QString());
    m_searchTerm.clear();

    m_view = nullptr;
    m_editor = editor;
}

void FindTextWidget::setWebView(WebView *view)
{
    // Reset search term and "matches found" label
    ui->lineEdit->setText(QString());
    ui->labelMatches->setText(QString());
    m_searchTerm.clear();

    m_editor = nullptr;
    m_view = view;
}

void FindTextWidget::resetEditorChanges()
{
    QTextDocument *doc = m_editor->document();
    QString contents = doc->toPlainText();
    int currentPos = m_editor->textCursor().position();

    emit pseudoModifiedDocument();
    doc->setPlainText(contents);

    QTextCursor c = m_editor->textCursor();
    c.setPosition(currentPos);
    m_editor->setTextCursor(c);
}

QLineEdit *FindTextWidget::getLineEdit()
{
    return ui->lineEdit;
}

void FindTextWidget::onFindNext()
{
    bool foundNext = false;
    if (m_view != nullptr)
    {
        auto flags = QFlags<QWebEnginePage::FindFlag>();

        if (m_matchCase)
            flags |= QWebEnginePage::FindCaseSensitively;

        m_view->findText(ui->lineEdit->text(), flags, [=](bool result){
            if (!result)
            {
                if (m_searchTerm.isNull() || ui->lineEdit->text().isNull())
                    ui->labelMatches->setText(QString());
                else
                    ui->labelMatches->setText(QString("Phrase not found"));
            }
            else
                setMatchCountLabel(true);
        });
        return;
    }
    else if (m_editor != nullptr)
        foundNext = findInEditor(true);

    if (foundNext)
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
    bool foundPrev = false;
    if (m_view != nullptr)
    {
        QFlags<QWebEnginePage::FindFlag> flags = QWebEnginePage::FindBackward;

        if (m_matchCase)
            flags |= QWebEnginePage::FindCaseSensitively;

        m_view->findText(ui->lineEdit->text(), flags, [=](bool result){
            if (!result)
            {
                if (m_searchTerm.isNull() || ui->lineEdit->text().isNull())
                    ui->labelMatches->setText(QString());
                else
                    ui->labelMatches->setText(QString("Phrase not found"));
            }
            else
                setMatchCountLabel(false);
        });
        return;
    }
    else if (m_editor != nullptr)
        foundPrev = findInEditor(false);

    if (foundPrev)
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
    if (m_editor != nullptr)
    {
        if (m_highlightAll)
            highlightAllInEditor(ui->lineEdit->text());
        else
            resetEditorChanges();
    }
}

void FindTextWidget::onToggleMatchCase(bool checked)
{
    m_matchCase = checked;
}

bool FindTextWidget::findInEditor(bool nextMatch)
{
    if (m_editor == nullptr)
        return false;

    if (ui->lineEdit->text().isEmpty())
    {
        resetEditorChanges();
        return false;
    }

    QString searchTerm = ui->lineEdit->text();
    if (searchTerm.compare(m_searchTerm) != 0 && !m_searchTerm.isEmpty())
        resetEditorChanges();

    QTextDocument::FindFlags flags = QTextDocument::FindFlags();
    if (!nextMatch)
        flags |= QTextDocument::FindBackward;
    if (m_matchCase)
        flags |= QTextDocument::FindCaseSensitively;

    // Look for search term
    QTextDocument *doc = m_editor->document();
    QTextCursor c = m_editor->textCursor();
    QTextCursor searchPos = doc->find(searchTerm, c, flags);

    // If highlight all option was selected, find all occurrences, and set position to prevPos
    if (m_highlightAll)
        highlightAllInEditor(searchTerm);

    // Try to wrap around text if term was not found
    if (searchPos.isNull())
    {
        int charCount = doc->characterCount();
        if (nextMatch && c.position() > 0)
        {
            c.setPosition(0);
            searchPos = doc->find(searchTerm, c, flags);
        }
        else if (!nextMatch && c.position() < charCount - 1)
        {
            c.setPosition(charCount - 1);
            searchPos = doc->find(searchTerm, c, flags);
        }
    }

    if (!searchPos.isNull())
    {
        m_editor->setTextCursor(searchPos);
        return true;
    }

    return false;
}

void FindTextWidget::highlightAllInEditor(const QString &term)
{
    if (!m_editor)
        return;

    QTextDocument *doc = m_editor->document();
    QTextCursor highlightCursor(doc);
    QTextCursor cursorPos = m_editor->textCursor();

    QTextCharFormat highlightFormat = cursorPos.charFormat();
    highlightFormat.setBackground(QBrush(QColor(245, 255, 91)));
    highlightFormat.setForeground(QBrush(QColor(0, 0, 0)));

    while (!highlightCursor.isNull() && !highlightCursor.atEnd())
    {
        highlightCursor = doc->find(term, highlightCursor);
        if (!highlightCursor.isNull())
        {
            emit pseudoModifiedDocument();
            highlightCursor.mergeCharFormat(highlightFormat);
        }
    }
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

        if (m_view != nullptr)
        {
            m_view->page()->toPlainText([=](const QString &result){
                setMatchCountLabelCallback(searchForNext, result);
            });
            return;
        }
        else if (m_editor != nullptr)
            setMatchCountLabelCallback(searchForNext, m_editor->document()->toPlainText());

        return;
    }

    // Update position counter
    int positionModifier = searchForNext ? 1 : -1;
    m_occurrenceCounter = (m_occurrenceCounter + positionModifier) % (m_numOccurrences + 1);
    if (m_occurrenceCounter == 0)
        m_occurrenceCounter = searchForNext ? 1 : m_numOccurrences;

    // Update the label
    ui->labelMatches->setText(QString("%1 of %2 matches").arg(m_occurrenceCounter).arg(m_numOccurrences));
}

void FindTextWidget::setMatchCountLabelCallback(bool searchForNext, const QString &documentText)
{
    Qt::CaseSensitivity caseSensitivity = m_matchCase ? Qt::CaseSensitive : Qt::CaseInsensitive;

    m_numOccurrences = documentText.count(m_searchTerm, caseSensitivity);
    
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

