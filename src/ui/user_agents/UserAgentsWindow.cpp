#include "UserAgentsWindow.h"
#include "ui_UserAgentsWindow.h"
#include "AddUserAgentDialog.h"
#include "UserAgentManager.h"

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QStandardItem>
#include <QStandardItemModel>

UserAgentsWindow::UserAgentsWindow(UserAgentManager *uaManager, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserAgentsWindow),
    m_userAgentManager(uaManager),
    m_model(new QStandardItemModel),
    m_addAgentDialog(nullptr)
{
    ui->setupUi(this);
    ui->treeView->setModel(m_model);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &UserAgentsWindow::saveAndClose);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &UserAgentsWindow::close);

    // "New" button actions
    QMenu *addMenu = new QMenu(ui->toolButtonAdd);
    addMenu->addAction(tr("Category"), this, &UserAgentsWindow::addCategory);
    addMenu->addAction(tr("User Agent"), this, &UserAgentsWindow::addUserAgent);
    ui->toolButtonAdd->setMenu(addMenu);

    // Edit user agent string action
    connect(ui->pushButtonEdit, &QPushButton::clicked, this, &UserAgentsWindow::editSelection);

    // Delete selection action
    connect(ui->pushButtonDelete, &QPushButton::clicked, this, &UserAgentsWindow::deleteSelection);

    // Edit name action
    connect(ui->treeView, &QTreeView::clicked, this, &UserAgentsWindow::onTreeItemClicked);
}

UserAgentsWindow::~UserAgentsWindow()
{
    delete ui;

    if (m_addAgentDialog)
        delete m_addAgentDialog;
}

void UserAgentsWindow::loadUserAgents()
{
    ui->pushButtonEdit->setEnabled(false);
    ui->pushButtonDelete->setEnabled(false);

    m_model->clear();
    m_model->setColumnCount(1);
    m_model->setHorizontalHeaderLabels(QStringList() << "User Agents");

    const UserAgent &activeAgent = m_userAgentManager->getUserAgent();

    // Load each UA category into the item model
    QStandardItem *parentItem = m_model->invisibleRootItem();
    for (auto it = m_userAgentManager->begin(); it != m_userAgentManager->end(); ++it)
    {
        QStandardItem *category = new QStandardItem(it.key());

        const std::vector<UserAgent> &agents = it.value();

        // Load agents belonging to the category
        for (const UserAgent &ua : agents)
        {
            QStandardItem *userAgent = new QStandardItem(ua.Name);
            userAgent->setData(ua.Value);

            bool isActiveAgent = (ua.Name.compare(activeAgent.Name) == 0
                                  && ua.Value.compare(activeAgent.Value) == 0);
            userAgent->setData(isActiveAgent, Qt::UserRole + 2);

            category->appendRow(userAgent);
        }

        parentItem->appendRow(category);
    }
}

void UserAgentsWindow::addCategory()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add a Category"), tr("Name:"), QLineEdit::Normal, QString(), &ok);
    if (ok && !name.isEmpty())
    {
        QStandardItem *category = new QStandardItem(name);
        m_model->invisibleRootItem()->appendRow(category);
    }
}

void UserAgentsWindow::addUserAgent()
{
    if (!m_addAgentDialog)
    {
        m_addAgentDialog = new AddUserAgentDialog;
        m_addAgentDialog->hideNewCategoryOption();

        connect(m_addAgentDialog, &AddUserAgentDialog::userAgentAdded, this, &UserAgentsWindow::onUserAgentAdded);
    }

    // Add category values to dialog every time it is shown - they could change any time the window is open
    m_addAgentDialog->clearAgentCategories();
    QStandardItem *rootItem = m_model->invisibleRootItem();
    int numCategories = rootItem->rowCount();
    for (int i = 0; i < numCategories; ++i)
    {
        QStandardItem *category = rootItem->child(i);
        m_addAgentDialog->addAgentCategory(category->text());
    }

    m_addAgentDialog->show();
}

void UserAgentsWindow::onUserAgentAdded()
{
    QString categoryName = m_addAgentDialog->getCategory();
    QString agentName = m_addAgentDialog->getAgentName();
    QString agentValue = m_addAgentDialog->getAgentValue();

    if (agentName.isEmpty() || agentValue.isEmpty())
        return;

    // Find category and append the user agent to the node
    QStandardItem *rootItem = m_model->invisibleRootItem();
    int numCategories = rootItem->rowCount();
    for (int i = 0; i < numCategories; ++i)
    {
        QStandardItem *category = rootItem->child(i);
        if (categoryName.compare(category->text()) == 0)
        {
            QStandardItem *item = new QStandardItem(agentName);
            item->setData(agentValue);

            category->appendRow(item);
        }
    }
}

void UserAgentsWindow::deleteSelection()
{
    QModelIndex selection = ui->treeView->currentIndex();
    if (!selection.isValid())
        return;

    QMessageBox::StandardButton answer = QMessageBox::question(this, tr("Confirm Deletion"),
                                                               tr("Are you sure you want to delete this item?"));
    if (answer == QMessageBox::Yes)
        m_model->removeRow(selection.row(), selection.parent());
}

void UserAgentsWindow::editSelection()
{
    QModelIndex selection = ui->treeView->currentIndex();
    if (!selection.isValid() || !selection.parent().isValid())
        return;

    QStandardItem *item = m_model->itemFromIndex(selection);
    if (!item)
        return;

    bool ok;
    QString agentString = QInputDialog::getText(this, tr("Edit User Agent"), tr("User Agent String:"),
                                         QLineEdit::Normal, item->data().toString(), &ok);
    if (ok && !agentString.isEmpty())
        item->setData(agentString);
}

void UserAgentsWindow::onTreeItemClicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    // Only enable edit button for user agents themselves, categories can be edited by double clicking the item
    bool enableEdit = index.parent().isValid();
    ui->pushButtonEdit->setEnabled(enableEdit);

    ui->pushButtonDelete->setEnabled(true);
}

void UserAgentsWindow::saveAndClose()
{
    m_userAgentManager->clearUserAgents();

    // Iterate through each category in the tree, updating the user agent manager with all changes made
    QStandardItem *rootItem = m_model->invisibleRootItem();
    QStandardItem *category = nullptr;
    QStandardItem *agent = nullptr;

    bool usingCustomUA = false;

    int numCategories = rootItem->rowCount();
    for (int i = 0; i < numCategories; ++i)
    {
        category = rootItem->child(i);
        std::vector<UserAgent> categoryAgents;

        int numAgents = category->rowCount();
        for (int j = 0; j < numAgents; ++j)
        {
            agent = category->child(j);

            UserAgent agentItem;
            agentItem.Name = agent->text();
            agentItem.Value = agent->data().toString();

            // Check if current item is supposed to be the active UA
            if (agent->data(Qt::UserRole + 2).toBool() == true)
            {
                m_userAgentManager->setActiveAgent(category->text(), agentItem);
                usingCustomUA = true;
            }

            categoryAgents.push_back(agentItem);
        }

        m_userAgentManager->addUserAgents(category->text(), std::move(categoryAgents));
    }

    // Reset to the default user agent if a custom one was in use, but was deleted by the user before saving changes
    if (!usingCustomUA)
        m_userAgentManager->disableActiveAgent();

    // Update browser windows
    m_userAgentManager->modifyWindowFinished();

    close();
}
