#include "AutoFillCredentialsView.h"
#include "ui_AutoFillCredentialsView.h"

#include "AutoFill.h"
#include "BrowserApplication.h"

#include <algorithm>
#include <functional>
#include <QMessageBox>
#include <QTableWidgetItem>

AutoFillCredentialsView::AutoFillCredentialsView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AutoFillCredentialsView),
    m_autoFill(sBrowserApplication->getAutoFill()),
    m_showingPasswords(false),
    m_credentials()
{
    ui->setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, true);

    populateTable();

    connect(ui->pushButtonRemove, &QPushButton::clicked, this, &AutoFillCredentialsView::onClickRemoveSelection);
    connect(ui->pushButtonRemoveAll, &QPushButton::clicked, this, &AutoFillCredentialsView::onClickRemoveAll);

    connect(ui->pushButtonTogglePasswords, &QPushButton::clicked, [this](){
        m_showingPasswords = !m_showingPasswords;

        populateTable();

        QString newButtonText = m_showingPasswords ? tr("Hide Passwords") : tr("Show Passwords");
        ui->pushButtonTogglePasswords->setText(newButtonText);
    });

    connect(ui->tableWidget, &QTableWidget::itemSelectionChanged, [this](){
        const bool hasSelection = !ui->tableWidget->selectedItems().isEmpty();
        ui->pushButtonRemove->setEnabled(hasSelection);
    });
}

AutoFillCredentialsView::~AutoFillCredentialsView()
{
    delete ui;
}

void AutoFillCredentialsView::onClickRemoveSelection()
{
    auto selection = ui->tableWidget->selectedItems();
    if (selection.isEmpty() || m_autoFill == nullptr)
        return;

    std::vector<int> selectedRows;
    for (const QTableWidgetItem *item : selection)
    {
        int row = item->row();
        if (std::find(selectedRows.begin(), selectedRows.end(), row) == selectedRows.end())
            selectedRows.push_back(row);
    }

    std::sort(selectedRows.begin(), selectedRows.end(), std::greater<int>());

    for (int row : selectedRows)
    {
        const WebCredentials &creds = m_credentials.at(row);
        m_autoFill->removeCredentials(creds);

        m_credentials.erase(m_credentials.begin() + row);

        ui->tableWidget->removeRow(row);
    }

    if (ui->tableWidget->selectedItems().isEmpty())
        ui->pushButtonRemove->setEnabled(false);

    if (m_credentials.empty())
        ui->pushButtonRemoveAll->setEnabled(false);
}

void AutoFillCredentialsView::onClickRemoveAll()
{
    if (!m_autoFill || m_credentials.empty())
        return;

    QMessageBox::StandardButton answer =
            QMessageBox::question(this, tr("Confirm choice"), tr("Do you want to delete all visible entries?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer == QMessageBox::No)
        return;

    ui->tableWidget->clearContents();

    // Only removing visible items, ie those stored in m_credentials
    for (const WebCredentials &creds : m_credentials)
        m_autoFill->removeCredentials(creds);

    m_credentials.clear();
}

void AutoFillCredentialsView::populateTable()
{
    QStringList headers;

    int columnCount = 3;
    if (m_showingPasswords)
    {
        headers = QStringList{ tr("Website"), tr("Username"), tr("Password"), tr("Last Login") };
        columnCount = 4;
    }
    else
        headers = QStringList{ tr("Website"), tr("Username"), tr("Last login") };

    ui->tableWidget->clearContents();
    ui->tableWidget->setColumnCount(columnCount);
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    if (isVisible())
    {
        for (int i = 0; i < columnCount; ++i)
            ui->tableWidget->setColumnWidth(i, (ui->tableWidget->width() / columnCount) - 1);
    }

    if (!m_autoFill)
        return;

    m_credentials = m_autoFill->getAllCredentials();
    if (m_credentials.empty())
        return;

    ui->pushButtonRemoveAll->setEnabled(true);

    ui->tableWidget->setRowCount(static_cast<int>(m_credentials.size()));

    int row = 0;
    for (const WebCredentials &creds: m_credentials)
    {
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(creds.Host));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(creds.Username));

        QTableWidgetItem *lastLogin
                = new QTableWidgetItem(creds.LastLogin.toString(QLatin1String("MMM d, yyyy, h:mm AP")));

        int column = 2;
        if (m_showingPasswords)
            ui->tableWidget->setItem(row, column++, new QTableWidgetItem(creds.Password));

        ui->tableWidget->setItem(row, column, lastLogin);

        ++row;
    }
}
