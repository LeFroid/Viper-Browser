#include "UserScriptEditor.h"
#include "ui_UserScriptEditor.h"
#include "BrowserApplication.h"
#include "CodeEditor.h"
#include "FindTextWidget.h"
#include "JavaScriptHighlighter.h"
#include "TextEditorTextFinder.h"

#include <QCloseEvent>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>

UserScriptEditor::UserScriptEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::UserScriptEditor),
    m_highlighter(new JavaScriptHighlighter),
    m_filePath(),
    m_rowIndex(-1),
    m_scriptModified(false),
    m_ignoreFindModify(false)
{
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    m_highlighter->setDocument(ui->scriptEditor->document());

    std::unique_ptr<ITextFinder> textFinder { std::make_unique<TextEditorTextFinder>() };
    static_cast<TextEditorTextFinder*>(textFinder.get())->setTextEdit(ui->scriptEditor);
    ui->widgetFindText->setTextFinder(std::move(textFinder));
    ui->widgetFindText->hide();

    if (sBrowserApplication->isDarkTheme())
    {
        ui->actionSave->setIcon(QIcon(QStringLiteral(":/document-save-white.png")));
        ui->actionFind->setIcon(QIcon(QStringLiteral(":/edit-find-white.png")));
    }

    connect(ui->widgetFindText, &FindTextWidget::pseudoModifiedDocument, this, &UserScriptEditor::onTextFindPseudoModify);
    connect(ui->scriptEditor, &CodeEditor::modificationChanged, this, &UserScriptEditor::onScriptModified);
    connect(ui->actionSave, &QAction::triggered, this, &UserScriptEditor::saveScript);
    connect(ui->actionFind, &QAction::triggered, this, &UserScriptEditor::toggleFindTextWidget);
    connect(ui->actionClose, &QAction::triggered, this, &UserScriptEditor::close);
}

UserScriptEditor::~UserScriptEditor()
{
    delete ui;
}

void UserScriptEditor::setScriptInfo(const QString &name, const QString &code, const QString &filePath, int rowIndex)
{
    setWindowTitle(QString("Editing %1").arg(name));
    ui->scriptEditor->setPlainText(code);

    ui->widgetFindText->clearLabels();

    m_filePath = filePath;
    m_scriptModified = false;
    m_rowIndex = rowIndex;
}

void UserScriptEditor::toggleFindTextWidget()
{
    if (ui->widgetFindText->isHidden())
        ui->widgetFindText->show();

    ui->widgetFindText->getLineEdit()->setFocus();
}

void UserScriptEditor::saveScript()
{
    QFile outFile(m_filePath);
    if (!outFile.open(QIODevice::WriteOnly))
        return;

    QByteArray scriptData;
    scriptData.append(ui->scriptEditor->toPlainText().toUtf8());
    outFile.write(scriptData);
    outFile.close();

    emit scriptModified(m_rowIndex);
    m_scriptModified = false;

    QString title = windowTitle();
    if (title.endsWith(QChar('*')))
        setWindowTitle(title.left(title.size() - 1));
}

void UserScriptEditor::onScriptModified(bool changed)
{
    if (m_ignoreFindModify)
    {
        m_ignoreFindModify = false;
        ui->scriptEditor->document()->setModified(false);
        return;
    }

    m_scriptModified = changed;
    QString title = windowTitle();
    if (changed)
    {
        if (!title.endsWith(QChar('*')))
            setWindowTitle(QString("%1*").arg(title));
    }
    else if (title.endsWith(QChar('*')))
        setWindowTitle(title.left(title.size() - 1));
}

void UserScriptEditor::onTextFindPseudoModify()
{
    m_ignoreFindModify = true;
}

void UserScriptEditor::closeEvent(QCloseEvent *event)
{
    if (!m_scriptModified)
    {
        event->accept();
        return;
    }

    QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("Script Editor"),
                                  tr("This script has been modified. Would you like to save before closing the script?"),
                                  QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                  QMessageBox::Yes);

    if (answer == QMessageBox::Cancel)
        event->ignore();
    else if (answer == QMessageBox::No)
        event->accept();
    else if (answer == QMessageBox::Yes)
    {
        saveScript();
        event->accept();
    }
}

