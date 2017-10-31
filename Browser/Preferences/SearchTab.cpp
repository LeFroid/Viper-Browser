#include "SearchTab.h"
#include "ui_SearchTab.h"

#include "AddSearchEngineDialog.h"
#include "SearchEngineManager.h"

SearchTab::SearchTab(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SearchTab),
    m_addEngineDialog(nullptr)
{
    ui->setupUi(this);
    ui->pushButtonRemoveEngine->setEnabled(false);

    loadSearchEngines();

    connect(ui->pushButtonAddEngine, &QPushButton::clicked, this, &SearchTab::showAddEngineDialog);
    connect(ui->pushButtonRemoveEngine, &QPushButton::clicked, this, &SearchTab::onRemoveSelectedEngine);
    connect(ui->comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SearchTab::onDefaultEngineChanged);
    connect(ui->listWidget, &QListWidget::currentRowChanged, [=](int index) {
        if (index >= 0)
            ui->pushButtonRemoveEngine->setEnabled(true);
        else
            ui->pushButtonRemoveEngine->setEnabled(false);
    });
}

SearchTab::~SearchTab()
{
    delete ui;
}

void SearchTab::onDefaultEngineChanged(int index)
{
    if (index < 0)
        return;

    SearchEngineManager::instance().setDefaultSearchEngine(ui->comboBox->itemText(index));
}

void SearchTab::onRemoveSelectedEngine()
{
    QListWidgetItem *item = ui->listWidget->currentItem();
    if (item == nullptr)
        return;

    QString engineName = item->text();
    SearchEngineManager::instance().removeSearchEngine(engineName);

    // Remove search engine from the list widget and combo box
    int comboBoxIdx = ui->comboBox->findText(engineName);
    if (comboBoxIdx >= 0)
        ui->comboBox->removeItem(comboBoxIdx);
    ui->listWidget->removeItemWidget(item);
    delete item;
}

void SearchTab::showAddEngineDialog()
{
    if (!m_addEngineDialog)
    {
        m_addEngineDialog = new AddSearchEngineDialog(this);
        connect(m_addEngineDialog, &AddSearchEngineDialog::searchEngineAdded, this, &SearchTab::addSearchEngine);
    }

    m_addEngineDialog->show();
    m_addEngineDialog->raise();
    m_addEngineDialog->activateWindow();
}

void SearchTab::addSearchEngine(const QString &name, const QString &queryUrl)
{
    SearchEngineManager::instance().addSearchEngine(name, queryUrl);
    ui->comboBox->addItem(name);
    ui->listWidget->addItem(name);
}

void SearchTab::loadSearchEngines()
{
    SearchEngineManager &manager = SearchEngineManager::instance();

    // Populate combo box and list widget simultaneously
    QList<QString> searchEngines = manager.getSearchEngineNames();
    const QString &defaultEngine = manager.getDefaultSearchEngine();
    int defaultEngineIdx = searchEngines.indexOf(defaultEngine);

    for (const QString &engine : searchEngines)
    {
        ui->comboBox->addItem(engine);
        ui->listWidget->addItem(engine);
    }

    ui->comboBox->setCurrentIndex(defaultEngineIdx);
}
