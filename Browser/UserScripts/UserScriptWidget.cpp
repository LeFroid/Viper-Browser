#include "UserScriptWidget.h"
#include "ui_UserScriptWidget.h"
#include "BrowserApplication.h"
#include "UserScriptManager.h"
#include "UserScriptModel.h"
#include "UserScriptTableView.h"


UserScriptWidget::UserScriptWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserScriptWidget)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    ui->setupUi(this);

    ui->tableViewScripts->setModel(sBrowserApplication->getUserScriptManager()->getModel());
}

UserScriptWidget::~UserScriptWidget()
{
    delete ui;
}
