#include "AddBookmarkDialog.h"
#include "ui_AddBookmarkDialog.h"

#include <QQueue>

AddBookmarkDialog::AddBookmarkDialog(std::shared_ptr<BookmarkManager> bookmarkMgr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddBookmarkDialog),
    m_bookmarkManager(bookmarkMgr),
    m_currentUrl()
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    // Populate combo box with up to 15 folders, by performing a BFS from the root folder
    int foldersAdded = 0;
    BookmarkFolder *f = nullptr;
    QQueue<BookmarkFolder*> q;
    q.enqueue(m_bookmarkManager->getRoot());
    while (!q.empty() && foldersAdded < 16)
    {
        f = q.dequeue();
        ui->comboBoxFolder->addItem(f->name, QVariant(f->id));
        for (BookmarkFolder *subF : f->folders)
            q.enqueue(subF);
        ++foldersAdded;
    }

    // Connect UI signals to slots
    connect(ui->pushButtonRemove, &QPushButton::clicked, this, &AddBookmarkDialog::onRemoveBookmark);
    connect(ui->pushButtonDone, &QPushButton::clicked, this, &AddBookmarkDialog::saveAndClose);
}

AddBookmarkDialog::~AddBookmarkDialog()
{
    delete ui;
}

void AddBookmarkDialog::setBookmarkInfo(const QString &bookmarkName, const QString &bookmarkUrl)
{
    m_currentUrl = bookmarkUrl;

    // Set name field
    ui->lineEditName->setText(bookmarkName);

    // New bookmarks are added by default to the root folder
    ui->comboBoxFolder->setCurrentIndex(0);
}

void AddBookmarkDialog::onRemoveBookmark()
{
    if (m_currentUrl.isEmpty())
        return;

    m_bookmarkManager->removeBookmark(m_currentUrl);
    emit updateBookmarkMenu();
    close();
}

void AddBookmarkDialog::saveAndClose()
{
    if (m_currentUrl.isEmpty())
        return;

    // Remove bookmark and re-add it with current name, url and parent folder values
    m_bookmarkManager->removeBookmark(m_currentUrl);
    m_bookmarkManager->addBookmark(ui->lineEditName->text(), m_currentUrl, ui->comboBoxFolder->currentData().toInt());
    emit updateBookmarkMenu();
    close();
}
