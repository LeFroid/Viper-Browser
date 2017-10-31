#include "ClearHistoryDialog.h"
#include "ui_clearhistorydialog.h"

#include <QStringList>

ClearHistoryDialog::ClearHistoryDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ClearHistoryDialog)
{
    ui->setupUi(this);
    layout()->setSizeConstraint(QLayout::SetFixedSize);

    setupListWidget();

    // Add options to time range combo box
    ui->comboBoxTimeRange->addItem(tr("Last Hour"), LAST_HOUR);
    ui->comboBoxTimeRange->addItem(tr("Last Two Hours"), LAST_TWO_HOUR);
    ui->comboBoxTimeRange->addItem(tr("Last Four Hours"), LAST_FOUR_HOUR);
    ui->comboBoxTimeRange->addItem(tr("Today"), LAST_DAY);
    ui->comboBoxTimeRange->addItem(tr("Everything"), TIME_RANGE_ALL);
    ui->comboBoxTimeRange->insertSeparator(4);

    // Set details hide/show icon and implement behavior
    ui->pushButtonDetails->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    connect(ui->pushButtonDetails, &QPushButton::clicked, this, &ClearHistoryDialog::toggleDetails);

    // Call accept() if the Apply button is clicked
    connect(ui->buttonBox, &QDialogButtonBox::clicked, [=](QAbstractButton *button){
        if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
            accept();
    });
}

ClearHistoryDialog::~ClearHistoryDialog()
{
    delete ui;
}

ClearHistoryDialog::TimeRangeOption ClearHistoryDialog::getTimeRange() const
{
    return static_cast<TimeRangeOption>(ui->comboBoxTimeRange->currentData().toInt());
}

HistoryType ClearHistoryDialog::getHistoryTypes() const
{
    HistoryType histTypes = HistoryType::None;

    // Fetch checkbox states
    QListWidgetItem *item = nullptr;
    for (int i = 0; i < ui->listWidgetDetails->count(); ++i)
    {
        item = ui->listWidgetDetails->item(i);
        histTypes = histTypes | item->data(Qt::UserRole).value<HistoryType>();
    }

    return histTypes;
}

void ClearHistoryDialog::toggleDetails()
{
    if (ui->listWidgetDetails->isHidden())
    {
        ui->listWidgetDetails->show();
        ui->pushButtonDetails->setIcon(style()->standardIcon(QStyle::SP_TitleBarMinButton));
    }
    else
    {
        ui->listWidgetDetails->hide();
        ui->pushButtonDetails->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    }
}

void ClearHistoryDialog::setupListWidget()
{
    // Add options to the list widget
    QStringList listOptions;
    listOptions << "Download & Browsing History"
                << "Form & Search History"
                << "Cookies"
                << "Cache";
    ui->listWidgetDetails->addItems(listOptions);

    // Make options checkable
    QListWidgetItem *item = nullptr;
    for (int i = 0; i < ui->listWidgetDetails->count(); ++i)
    {
        item = ui->listWidgetDetails->item(i);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
    }

    // Associate user data type (HistoryTypes) with widget items
    item = ui->listWidgetDetails->item(0);
    item->setData(Qt::UserRole, static_cast<uint32_t>(HistoryType::Browsing));
    item = ui->listWidgetDetails->item(1);
    item->setData(Qt::UserRole, static_cast<uint32_t>(HistoryType::Search));
    item = ui->listWidgetDetails->item(2);
    item->setData(Qt::UserRole, static_cast<uint32_t>(HistoryType::Cookies));
    item = ui->listWidgetDetails->item(3);
    item->setData(Qt::UserRole, static_cast<uint32_t>(HistoryType::Cache));
}
