#include "AdBlockLogDisplay.h"
#include "ui_AdBlockLogDisplay.h"

#include "AdBlockLog.h"
#include "AdBlockLogTableModel.h"
#include "AdBlockManager.h"

#include <QSortFilterProxyModel>

AdBlockLogDisplay::AdBlockLogDisplay(adblock::AdBlockManager *adBlockManager) :
    QWidget(nullptr),
    ui(new Ui::AdBlockLogDisplay),
    m_adBlockManager(adBlockManager),
    m_proxyModel(new QSortFilterProxyModel(this)),
    m_sourceModel(new adblock::LogTableModel(this)),
    m_logSource(LogSourcePageUrl),
    m_sourceUrl()
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

    // Setup push button binding
    connect(ui->pushButtonReload, &QPushButton::clicked, this, &AdBlockLogDisplay::onReloadClicked);

    // Setup combo box for log source type
    ui->comboBoxLogSource->addItem(tr("URL"), QVariant(static_cast<int>(LogSourcePageUrl)));
    ui->comboBoxLogSource->addItem(tr("All URLs"), QVariant(static_cast<int>(LogSourceAll)));
    connect(ui->comboBoxLogSource, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &AdBlockLogDisplay::onComboBoxIndexChanged);
}

AdBlockLogDisplay::~AdBlockLogDisplay()
{
    delete ui;
}

void AdBlockLogDisplay::setLogTableFor(const QUrl &url)
{
    m_sourceUrl = url;
    ui->comboBoxLogSource->setCurrentIndex(0);
    ui->comboBoxLogSource->setItemText(0, url.toString());
    m_sourceModel->setLogEntries(m_adBlockManager->getLog()->getEntriesFor(url));
    ui->tableView->resizeColumnsToContents();
}

void AdBlockLogDisplay::showAllLogs()
{
    ui->comboBoxLogSource->setCurrentIndex(1);
    m_sourceModel->setLogEntries(m_adBlockManager->getLog()->getAllEntries());
    ui->tableView->resizeColumnsToContents();
}

void AdBlockLogDisplay::onSearchTermEntered()
{
    m_proxyModel->setFilterFixedString(ui->lineEditSearch->text());
}

void AdBlockLogDisplay::onReloadClicked()
{
    switch (m_logSource)
    {
        case LogSourceAll:
            showAllLogs();
            break;
        case LogSourcePageUrl:
            setLogTableFor(m_sourceUrl);
            break;
    }
}

void AdBlockLogDisplay::onComboBoxIndexChanged(int index)
{
    LogSource sourceType = static_cast<LogSource>(ui->comboBoxLogSource->itemData(index).toInt());
    if (sourceType == m_logSource)
        return;

    switch (sourceType)
    {
        case LogSourceAll:
            showAllLogs();
            break;
        case LogSourcePageUrl:
            setLogTableFor(m_sourceUrl);
            break;
    }

    m_logSource = sourceType;
}
