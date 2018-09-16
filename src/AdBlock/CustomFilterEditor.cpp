#include "CustomFilterEditor.h"
#include "ui_CustomFilterEditor.h"
#include "BrowserApplication.h"
#include "CodeEditor.h"
#include "FindTextWidget.h"
#include "Settings.h"

#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
CustomFilterEditor::CustomFilterEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::CustomFilterEditor),
    m_filePath(),
    m_filtersModified(false),
    m_ignoreFindModify(false)
{
    setAttribute(Qt::WA_DeleteOnClose);

    ui->setupUi(this);

    m_filePath = sBrowserApplication->getSettings()->getPathValue(BrowserSetting::AdBlockPlusDataDir);
    m_filePath.append(QDir::separator());
    m_filePath.append(QLatin1String("custom.txt"));

    ui->widgetFindText->hide();

    connect(ui->widgetFindText, &FindTextWidget::pseudoModifiedDocument, this, &CustomFilterEditor::onTextFindPseudoModify);
    connect(ui->filterEditor, &CodeEditor::modificationChanged, this, &CustomFilterEditor::onFiltersModified);
    connect(ui->actionSave, &QAction::triggered, this, &CustomFilterEditor::saveFilters);
    connect(ui->actionFind, &QAction::triggered, this, &CustomFilterEditor::toggleFindTextWidget);
    connect(ui->actionClose, &QAction::triggered, this, &CustomFilterEditor::close);

    loadUserFilters();
}

CustomFilterEditor::~CustomFilterEditor()
{
    delete ui;
}

void CustomFilterEditor::toggleFindTextWidget()
{
    if (ui->widgetFindText->isHidden())
    {
        ui->widgetFindText->setTextEdit(ui->filterEditor);
        ui->widgetFindText->show();
    }
    ui->widgetFindText->getLineEdit()->setFocus();
}

void CustomFilterEditor::saveFilters()
{
    QFile outFile(m_filePath);
    if (!outFile.open(QIODevice::WriteOnly))
        return;

    QByteArray filterData;
    filterData.append(ui->filterEditor->toPlainText());
    outFile.write(filterData);
    outFile.close();

    emit filtersModified();

    m_filtersModified = false;
    QString title = windowTitle();
    if (title.endsWith(QChar('*')))
        setWindowTitle(title.left(title.size() - 1));
}

void CustomFilterEditor::onFiltersModified(bool changed)
{
    if (m_ignoreFindModify)
    {
        m_ignoreFindModify = false;
        ui->filterEditor->document()->setModified(false);
        return;
    }

    m_filtersModified = changed;
    QString title = windowTitle();
    if (changed)
    {
        if (!title.endsWith(QChar('*')))
            setWindowTitle(QString("%1*").arg(title));
    }
    else if (title.endsWith(QChar('*')))
        setWindowTitle(title.left(title.size() - 1));
}

void CustomFilterEditor::onTextFindPseudoModify()
{
    m_ignoreFindModify = true;
}

void CustomFilterEditor::closeEvent(QCloseEvent *event)
{
    if (!m_filtersModified)
    {
        event->accept();
        return;
    }

    QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("Filter Editor"),
                                  tr("Your blocking filters have been modified. Would you like to save before closing?"),
                                  QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
                                  QMessageBox::Yes);

    if (answer == QMessageBox::Cancel)
        event->ignore();
    else if (answer == QMessageBox::No)
        event->accept();
    else if (answer == QMessageBox::Yes)
    {
        saveFilters();
        event->accept();
    }
}

void CustomFilterEditor::loadUserFilters()
{
    QFile filterFile(m_filePath);
    if (!filterFile.exists())
        emit createUserSubscription();

    if (filterFile.open(QIODevice::ReadOnly))
        ui->filterEditor->setPlainText(QString(filterFile.readAll()));

    ui->widgetFindText->setTextEdit(ui->filterEditor);
    m_filtersModified = false;
}

