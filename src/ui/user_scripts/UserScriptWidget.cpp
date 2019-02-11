#include "UserScriptWidget.h"
#include "ui_UserScriptWidget.h"
#include "AddUserScriptDialog.h"
#include "UserScriptManager.h"
#include "UserScriptModel.h"
#include "UserScriptEditor.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <algorithm>
#include <vector>

UserScriptWidget::UserScriptWidget(UserScriptManager *userScriptManager) :
    QWidget(nullptr),
    ui(new Ui::UserScriptWidget),
    m_userScriptManager(userScriptManager)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

    ui->tableViewScripts->setModel(m_userScriptManager->getModel());

    connect(m_userScriptManager,         &UserScriptManager::scriptCreated, this, &UserScriptWidget::onScriptCreated);

    connect(ui->tableViewScripts,        &CheckableTableView::clicked,  this, &UserScriptWidget::onItemClicked);
    connect(ui->pushButtonInstallScript, &QPushButton::clicked,         this, &UserScriptWidget::onInstallButtonClicked);
    connect(ui->pushButtonCreateScript,  &QPushButton::clicked,         this, &UserScriptWidget::onCreateButtonClicked);
    connect(ui->pushButtonDeleteScript,  &QPushButton::clicked,         this, &UserScriptWidget::onDeleteButtonClicked);
    connect(ui->pushButtonEditScript,    &QPushButton::clicked,         this, &UserScriptWidget::onEditButtonClicked);
}

void UserScriptWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int tableWidth = ui->tableViewScripts->geometry().width();
    ui->tableViewScripts->setColumnWidth(0, tableWidth / 25);
    ui->tableViewScripts->setColumnWidth(1, tableWidth / 4);
    ui->tableViewScripts->setColumnWidth(2, tableWidth / 2);
    ui->tableViewScripts->setColumnWidth(3, tableWidth / 5);
}

UserScriptWidget::~UserScriptWidget()
{
    delete ui;
}

void UserScriptWidget::onItemClicked(const QModelIndex &/*index*/)
{
    ui->pushButtonEditScript->setEnabled(true);
    ui->pushButtonDeleteScript->setEnabled(true);
}

void UserScriptWidget::onDeleteButtonClicked()
{
    QModelIndexList selection = ui->tableViewScripts->selectionModel()->selectedIndexes();
    if (selection.empty())
        return;

    // Get user confirmation before deleting items
    QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("Confirm"), tr("Are you sure you want to delete the selected script(s)?"),
                                  QMessageBox::Yes|QMessageBox::No);
    if (answer == QMessageBox::No)
        return;

    // Get rows from selection without duplicates (in case multiple columns of same row(s) are selected)
    std::vector<int> selectedRows;
    for (const QModelIndex &idx : selection)
    {
        int row = idx.row();
        auto it = std::find(selectedRows.begin(), selectedRows.end(), row);
        if (it == selectedRows.end())
            selectedRows.push_back(row);
    }
    // Sort rows in descending order
    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());

    // Remove scripts
    UserScriptModel *model = qobject_cast<UserScriptModel*>(ui->tableViewScripts->model());
    for (int row : selectedRows)
        model->removeRow(row);
}

void UserScriptWidget::onEditButtonClicked()
{
    QModelIndex idx = ui->tableViewScripts->selectionModel()->currentIndex();
    if (!idx.isValid())
        return;

    UserScriptModel *model = qobject_cast<UserScriptModel*>(ui->tableViewScripts->model());
    int rowIndex = idx.row();
    QModelIndex nameIdx = model->index(rowIndex, 1);
    QString scriptName = model->data(nameIdx, Qt::DisplayRole).toString();

    UserScriptEditor *editor = new UserScriptEditor;
    connect(editor, &UserScriptEditor::scriptModified, model, &UserScriptModel::reloadScript);
    editor->setScriptInfo(scriptName, model->getScriptSource(rowIndex), model->getScriptFileName(rowIndex), rowIndex);
    editor->show();
}

void UserScriptWidget::onInstallButtonClicked()
{
    bool ok;
    QString userInput = QInputDialog::getText(this, tr("Install User Script"), tr("Enter URL of User Script:"),
                                              QLineEdit::Normal, QString(), &ok);
    if (!ok || userInput.isEmpty())
        return;

    QUrl scriptUrl = QUrl::fromUserInput(userInput);
    if (!scriptUrl.isValid())
    {
        static_cast<void>(QMessageBox::warning(this, tr("Installation Error"), tr("Could not install script."), QMessageBox::Ok,
                                               QMessageBox::Ok));
        return;
    }
    m_userScriptManager->installScript(scriptUrl);
}

void UserScriptWidget::onCreateButtonClicked()
{    
    AddUserScriptDialog *scriptDialog = new AddUserScriptDialog;
    connect(scriptDialog, &AddUserScriptDialog::informationEntered, m_userScriptManager, &UserScriptManager::createScript);
    scriptDialog->show();
    scriptDialog->setFocus();
    scriptDialog->activateWindow();
}

void UserScriptWidget::onScriptCreated(int scriptIdx)
{
    UserScriptModel *model = qobject_cast<UserScriptModel*>(ui->tableViewScripts->model());
    QModelIndex nameIdx = model->index(scriptIdx, 1);
    QString scriptName = model->data(nameIdx, Qt::DisplayRole).toString();

    UserScriptEditor *editor = new UserScriptEditor;
    connect(editor, &UserScriptEditor::scriptModified, model, &UserScriptModel::reloadScript);
    editor->setScriptInfo(scriptName, model->getScriptSource(scriptIdx), model->getScriptFileName(scriptIdx), scriptIdx);
    editor->show();
}
