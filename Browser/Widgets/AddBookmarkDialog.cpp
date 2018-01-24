#include "AddBookmarkDialog.h"
#include "ui_AddBookmarkDialog.h"
#include "BookmarkNode.h"

#include <QQueue>

AddBookmarkDialog::AddBookmarkDialog(BookmarkManager *bookmarkMgr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddBookmarkDialog),
    m_bookmarkManager(bookmarkMgr),
    m_currentUrl()
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);

    // Populate combo box with each folder in the bookmark collection
    bool isRoot = true;
    BookmarkNode *f = nullptr;
    QQueue<BookmarkNode*> q;
    q.enqueue(m_bookmarkManager->getRoot());
    while (!q.empty())
    {
        f = q.dequeue();
        if (isRoot)
            isRoot = false;
        else
            ui->comboBoxFolder->addItem(f->getName(), qVariantFromValue((void *)f));
        int numChildren = f->getNumChildren();
        for (int i = 0; i < numChildren; ++i)
        {
            BookmarkNode *n = f->getNode(i);
            if (n->getType() == BookmarkNode::Folder)
                q.enqueue(n);
        }
    }
    ui->comboBoxFolder->setCurrentIndex(0);

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
    BookmarkNode *parentNode = (BookmarkNode*) ui->comboBoxFolder->currentData().value<void*>();
    m_bookmarkManager->appendBookmark(ui->lineEditName->text(), m_currentUrl, parentNode);
    emit updateBookmarkMenu();
    close();
}
