#include "BookmarkDialog.h"
#include "ui_BookmarkDialog.h"
#include "BookmarkNode.h"
#include "BookmarkManager.h"

#include <QtConcurrent>
#include <QFontMetrics>
#include <QFuture>
#include <QQueue>

BookmarkDialog::BookmarkDialog(BookmarkManager *bookmarkMgr, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BookmarkDialog),
    m_bookmarkManager(bookmarkMgr),
    m_currentUrl()
{
    ui->setupUi(this);

    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_X11NetWmWindowTypeCombo, true);
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);
    setContentsMargins(0, 0, 0, 0);

    const int maxTextWidth = std::max(width(), 290) * 3 / 4;
    QFontMetrics folderFontMetrics(font());
    // Populate combo box with each folder in the bookmark collection
    const BookmarkNode *rootNode = m_bookmarkManager->getRoot();
    for (const auto it : *m_bookmarkManager)
    {
        if (it != rootNode && it->getType() == BookmarkNode::Folder)
            ui->comboBoxFolder->addItem(folderFontMetrics.elidedText(it->getName(), Qt::ElideRight, maxTextWidth) , QVariant::fromValue((void *)it));
    }

    ui->comboBoxFolder->setCurrentIndex(0);

    // Connect UI signals to slots
    connect(ui->pushButtonRemove, &QPushButton::clicked, this, &BookmarkDialog::onRemoveBookmark);
    connect(ui->pushButtonDone,   &QPushButton::clicked, this, &BookmarkDialog::saveAndClose);
}

BookmarkDialog::~BookmarkDialog()
{
    delete ui;
}

void BookmarkDialog::alignAndShow(const QRect &windowGeom, const QRect &toolbarGeom, const QRect &urlBarGeom)
{
    QPoint dialogPos;
    dialogPos.setX(windowGeom.x() + toolbarGeom.x() + urlBarGeom.x() + urlBarGeom.width() - (width() / 2));
    dialogPos.setY(windowGeom.y() + toolbarGeom.y() + toolbarGeom.height() + (urlBarGeom.height() * 2 / 3));

    move(dialogPos);

    //setMaximumWidth(windowGeom.width() / 7);

    show();
    activateWindow();
    raise();

    ui->lineEditName->setFocus();
}

void BookmarkDialog::setDialogHeader(const QString &text)
{
    setWindowTitle(text);
    ui->labelDialogHeader->setText(text);
}

void BookmarkDialog::setBookmarkInfo(const QString &bookmarkName, const QUrl &bookmarkUrl, BookmarkNode *parentFolder)
{
    m_currentUrl = bookmarkUrl;

    // Set name field
    ui->lineEditName->setText(bookmarkName);

    // Set parent in combobox if parentFolder was specified
    if (parentFolder != nullptr)
    {
        const int numFolders = ui->comboBoxFolder->count();
        for (int i = 0; i < numFolders; ++i)
        {
            BookmarkNode *itemPtr = static_cast<BookmarkNode*>(ui->comboBoxFolder->itemData(i, Qt::UserRole).value<void*>());
            if (itemPtr && itemPtr == parentFolder)
            {
                ui->comboBoxFolder->setCurrentIndex(i);
                return;
            }
        }
    }
}

void BookmarkDialog::onRemoveBookmark()
{
    if (m_currentUrl.isEmpty())
        return;

    m_bookmarkManager->removeBookmark(m_currentUrl);
    close();
}

void BookmarkDialog::saveAndClose()
{
    if (m_currentUrl.isEmpty())
    {
        close();
        return;
    }   

	QFuture<void> f = QtConcurrent::run([this]() {

        BookmarkNode *parentNode = (BookmarkNode*)ui->comboBoxFolder->currentData().value<void*>();
        if (m_bookmarkManager->isBookmarked(m_currentUrl))
        {
            if (BookmarkNode *n = m_bookmarkManager->getBookmark(m_currentUrl))
            {
                if (n->getName() != ui->lineEditName->text())
                    m_bookmarkManager->setBookmarkName(n, ui->lineEditName->text());

                if (n->getParent() != parentNode)
                    m_bookmarkManager->setBookmarkParent(n, parentNode);
            }
        }
        else
            m_bookmarkManager->appendBookmark(ui->lineEditName->text(), m_currentUrl, parentNode);
	});
    close();
}
