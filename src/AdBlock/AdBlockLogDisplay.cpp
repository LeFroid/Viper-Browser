#include "AdBlockLogDisplay.h"
#include "ui_AdBlockLogDisplay.h"

#include "AdBlockLog.h"
#include "AdBlockLogTableModel.h"
#include "AdBlockManager.h"

#include <QSortFilterProxyModel>

AdBlockLogDisplay::AdBlockLogDisplay(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AdBlockLogDisplay),
    m_proxyModel(new QSortFilterProxyModel(this)),
    m_sourceModel(new AdBlockLogTableModel(this))
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);

    // Setup proxy model and table view
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setFilterKeyColumn(-1); // -1 applies search terms to all columns

    m_proxyModel->setSourceModel(m_sourceModel);
    ui->tableView->setModel(m_proxyModel);

    // Setup search event handler
    connect(ui->lineEditSearch, &QLineEdit::editingFinished, this, &AdBlockLogDisplay::onSearchTermEntered);

    //todo: implement reload feature and handle combobox (options for: 1) logs for origin = current url, 2) logs for all urls)
}

AdBlockLogDisplay::~AdBlockLogDisplay()
{
    delete ui;
}

void AdBlockLogDisplay::setLogTableFor(const QUrl &url)
{
    ui->comboBoxLogSource->setCurrentText(url.toString());
    m_sourceModel->setLogEntries(AdBlockManager::instance().getLog()->getEntriesFor(url));
    ui->tableView->resizeColumnsToContents();
}

void AdBlockLogDisplay::onSearchTermEntered()
{
    m_proxyModel->setFilterRegExp(ui->lineEditSearch->text());
}
