#include "FileChooserLineEdit.h"

#include <QDir>
#include <QResizeEvent>
#include <QToolButton>

FileChooserLineEdit::FileChooserLineEdit(QWidget *parent) :
    QLineEdit(parent),
    m_buttonBrowse(new QToolButton(this)),
    m_fileMode(QFileDialog::AnyFile)
{
    m_buttonBrowse->setText(tr("Browse..."));
    connect(m_buttonBrowse, &QToolButton::clicked, this, &FileChooserLineEdit::onClickBrowse);

    setStyleSheet(QString("QLineEdit { padding: 1px %1px 1px 0px; }").arg(m_buttonBrowse->sizeHint().width()));
}

void FileChooserLineEdit::setFileMode(QFileDialog::FileMode mode)
{
    // Do not support multiple file selection
    if (mode == QFileDialog::ExistingFiles)
        return;

    m_fileMode = mode;
}

void FileChooserLineEdit::onClickBrowse()
{
    QString result;
    switch (m_fileMode)
    {
        case QFileDialog::AnyFile:
            result = QFileDialog::getSaveFileName(this, tr("Select a file"), QDir::homePath());
            break;
        case QFileDialog::ExistingFile:
            result = QFileDialog::getOpenFileName(this, tr("Select a file"), QDir::homePath());
            break;
        case QFileDialog::Directory:
            result = QFileDialog::getExistingDirectory(this, tr("Select a directory"), QDir::homePath());
            break;
        case QFileDialog::ExistingFiles:
        default:
            break;
    }
    setText(result);
}

void FileChooserLineEdit::resizeEvent(QResizeEvent *event)
{
    QLineEdit::resizeEvent(event);

    QSize btnSize = m_buttonBrowse->sizeHint();
    const QRect widgetRect = rect();
    m_buttonBrowse->move(widgetRect.right() + 1 - btnSize.width(), widgetRect.top());
}
