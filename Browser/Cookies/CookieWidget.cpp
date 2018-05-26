#include "BrowserApplication.h"
#include "CookieJar.h"
#include "CookieWidget.h"
#include "ui_cookiewidget.h"
#include "CookieModifyDialog.h"
#include "CookieTableModel.h"
#include "DetailedCookieTableModel.h"
#include "NetworkAccessManager.h"

#include <QList>
#include <QMessageBox>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

CookieWidget::CookieWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CookieWidget),
    m_cookieDialog(new CookieModifyDialog(this)),
    m_dialogEditMode(false)
{
    ui->setupUi(this);

    ui->tableViewCookies->setModel(new CookieTableModel(this));
    ui->tableViewCookieDetail->setModel(new DetailedCookieTableModel(this));

    // Enable search for cookies
    connect(ui->lineEditSearch, &QLineEdit::returnPressed, this, &CookieWidget::searchCookies);

    // Connect change in selection in top table to change in data display in bottom table
    connect(ui->tableViewCookies, &QTableView::clicked, this, &CookieWidget::onCookieClicked);

    // Close window when close button is clicked
    connect(ui->pushButtonClose, &QPushButton::clicked, this, &CookieWidget::close);

    // Delete cookie(s) action
    connect(ui->pushButtonDeleteCookie, &QPushButton::clicked, this, &CookieWidget::onDeleteClicked);

    // Cookie dialog slots
    connect(ui->pushButtonNewCookie, &QPushButton::clicked, this, &CookieWidget::onNewCookieClicked);
    connect(ui->pushButtonEditCookie, &QPushButton::clicked, this, &CookieWidget::onEditCookieClicked);
    connect(m_cookieDialog, &CookieModifyDialog::finished, this, &CookieWidget::onCookieDialogFinished);
}

CookieWidget::~CookieWidget()
{
    delete ui;
}

void CookieWidget::resetUI()
{
    static_cast<CookieTableModel*>(ui->tableViewCookies->model())->loadCookies();
}

void CookieWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    int tableWidth = ui->tableViewCookies->geometry().width();
    ui->tableViewCookies->setColumnWidth(0, tableWidth / 25);
    ui->tableViewCookies->setColumnWidth(1, tableWidth * 45 / 100);
    ui->tableViewCookies->setColumnWidth(2, tableWidth * 45 / 100);

    ui->tableViewCookieDetail->setColumnWidth(0, tableWidth / 4);
    ui->tableViewCookieDetail->setColumnWidth(1, tableWidth * 3 / 4);
}

void CookieWidget::searchCookies()
{
    CookieTableModel *model = static_cast<CookieTableModel*>(ui->tableViewCookies->model());
    model->searchFor(ui->lineEditSearch->text());

    // Clear info in detailed cookie view
    static_cast<DetailedCookieTableModel*>(ui->tableViewCookieDetail->model())->setCookie(QNetworkCookie());
}

void CookieWidget::onCookieClicked(const QModelIndex &index)
{
    CookieTableModel *model = static_cast<CookieTableModel*>(ui->tableViewCookies->model());
    QNetworkCookie cookie = model->getCookie(index);
    static_cast<DetailedCookieTableModel*>(ui->tableViewCookieDetail->model())->setCookie(cookie);
}

void CookieWidget::onNewCookieClicked()
{
    if (m_cookieDialog->isVisible())
        return;

    m_dialogEditMode = false;

    QNetworkCookie cookie;
    m_cookieDialog->setCookie(cookie);
    m_cookieDialog->show();
}

void CookieWidget::onEditCookieClicked()
{
    if (m_cookieDialog->isVisible())
        return;

    m_dialogEditMode = true;

    // Get current cookie selection from table model
    CookieTableModel *model = static_cast<CookieTableModel*>(ui->tableViewCookies->model());
    QNetworkCookie cookie = model->getCookie(ui->tableViewCookies->currentIndex());
    m_cookieDialog->setCookie(cookie);
    m_cookieDialog->show();
}

void CookieWidget::onDeleteClicked()
{
    CookieTableModel *model = static_cast<CookieTableModel*>(ui->tableViewCookies->model());

    // Determine which rows (if any) are checked
    const QList<int> &checkedStates = model->getCheckedStates();
    QList<int> checkedRows;
    for (int i = 0; i < checkedStates.size(); ++i)
    {
        if (checkedStates.at(i) == Qt::Checked)
            checkedRows.append(i);
    }

    // If no rows are checked, delete the current selection
    if (checkedRows.empty())
    {
        QModelIndex index = ui->tableViewCookies->currentIndex();
        if (index.isValid())
            model->removeRow(index.row());
        return;
    }

    QMessageBox confirmBox;
    confirmBox.setText(tr("Please Confirm"));
    confirmBox.setInformativeText(QString("You are about to delete %1 cookie(s)").arg(checkedRows.size()));
    confirmBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    confirmBox.setDefaultButton(QMessageBox::Cancel);
    int choice = confirmBox.exec();
    if (choice == QMessageBox::Ok)
    {
        for (int i = 0; i < checkedRows.size(); ++i)
        {
            int row = checkedRows.at(i) - i;
            model->removeRow(row);
        }
    }
}

void CookieWidget::onCookieDialogFinished(int result)
{
    if (result == QDialog::Rejected)
        return;

    //CookieTableModel *model = static_cast<CookieTableModel*>(ui->tableViewCookies->model());
    //CookieJar *jar = sBrowserApplication->getCookieJar();
    auto cookieStore = QWebEngineProfile::defaultProfile()->cookieStore();
    const QNetworkCookie &cookie = m_cookieDialog->getCookie();
    cookieStore->setCookie(cookie);
    /*if (m_dialogEditMode)
    {

        if (!jar->updateCookie(cookie))
        {
            jar->insertCookie(cookie);
            model->insertRow(model->rowCount());
        }
        else
            static_cast<DetailedCookieTableModel*>(ui->tableViewCookieDetail->model())->setCookie(cookie);
    }
    else
    {
        jar->insertCookie(cookie);
        model->insertRow(model->rowCount());
    }*/
}
