#include "BrowserApplication.h"
#include "URLSuggestionItemDelegate.h"
#include "URLSuggestionListModel.h"
#include "URLSuggestionWidget.h"
#include "URLSuggestionWorker.h"
#include "URLLineEdit.h"

#include <QKeyEvent>
#include <QListView>
#include <QTimer>
#include <QVBoxLayout>

#include "BrowserApplication.h"
#include "FaviconStorage.h"
#include <QUrl>
#include <vector>
#include <QDebug>

URLSuggestionWidget::URLSuggestionWidget(QWidget *parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setAttribute(Qt::WA_X11NetWmWindowTypeCombo, true);
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);
    setContentsMargins(0, 0, 0, 0);

    m_suggestionList = new QListView(this);
    m_suggestionList->setSelectionBehavior(QListView::SelectRows);
    m_suggestionList->setSelectionMode(QListView::SingleSelection);
    m_suggestionList->setEditTriggers(QListView::NoEditTriggers);
    m_suggestionList->setItemDelegate(new URLSuggestionItemDelegate(this));
    connect(m_suggestionList, &QListView::clicked, [=](const QModelIndex &index){
        if (index.isValid())
        {
            hide();
            emit urlChosen(QUrl(index.data(URLSuggestionListModel::Link).toString()));
        }
    });

    m_model = new URLSuggestionListModel(this);
    m_suggestionList->setModel(m_model);

    m_worker = new URLSuggestionWorker(this);
    connect(m_worker, &URLSuggestionWorker::finishedSearch, m_model, &URLSuggestionListModel::setSuggestions);

    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->setSpacing(0);
    vboxLayout->setContentsMargins(0, 0, 0, 0);

    vboxLayout->addWidget(m_suggestionList);

    sBrowserApplication->installEventFilter(this);
}

bool URLSuggestionWidget::eventFilter(QObject *watched, QEvent *event)
{
    // Mostly taken from QCompleter
    if (!isVisible() || watched == this || watched == m_suggestionList)
        return QObject::eventFilter(watched, event);

    switch(event->type())
    {
        case QEvent::KeyPress:
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            int key = keyEvent->key();

            QModelIndex currentIndex = m_suggestionList->currentIndex();

            if (key == Qt::Key_Tab || key == Qt::Key_Backtab)
            {
                if ((keyEvent->modifiers() & (~Qt::ShiftModifier)) != Qt::NoModifier)
                    return true;

                const bool shiftMod = keyEvent->modifiers() == Qt::ShiftModifier;
                if (key == Qt::Key_Backtab || (key == Qt::Key_Tab && shiftMod))
                    key = Qt::Key_Up;
                else
                    key = Qt::Key_Down;
            }

            switch (key)
            {
                case Qt::Key_End:
                case Qt::Key_Home:
                    if (keyEvent->modifiers() & Qt::ControlModifier)
                        return false;
                    break;
                case Qt::Key_Up:
                {
                    // Shift selection up by 1
                    if (!currentIndex.isValid())
                    {
                        const int rowCount = m_model->rowCount();
                        QModelIndex prevIndex = m_model->index(rowCount - 1, 0);
                        m_suggestionList->setCurrentIndex(prevIndex);
                        m_lineEdit->setURL(QUrl(prevIndex.data(URLSuggestionListModel::Link).toString()));
                    }
                    else if (currentIndex.row() > 0)
                    {
                        QModelIndex prevIndex = m_model->index(currentIndex.row() - 1, 0);
                        m_suggestionList->setCurrentIndex(prevIndex);
                        m_lineEdit->setURL(QUrl(prevIndex.data(URLSuggestionListModel::Link).toString()));
                    }
                    else
                    {
                        m_suggestionList->setCurrentIndex(QModelIndex());
                        m_lineEdit->setURL(QUrl());
                    }
                    return true;
                }
                case Qt::Key_Down:
                {
                    // Shift selection down by 1
                    if (!currentIndex.isValid())
                    {
                        QModelIndex firstIndex = m_model->index(0, 0);
                        m_suggestionList->setCurrentIndex(firstIndex);
                        m_lineEdit->setURL(QUrl(firstIndex.data(URLSuggestionListModel::Link).toString()));
                    }
                    else if (currentIndex.row() < m_model->rowCount() - 1)
                    {
                        QModelIndex nextIndex = m_model->index(currentIndex.row() + 1, 0);
                        m_suggestionList->setCurrentIndex(nextIndex);
                        m_lineEdit->setURL(QUrl(nextIndex.data(URLSuggestionListModel::Link).toString()));
                    }
                    else
                    {
                        m_suggestionList->setCurrentIndex(QModelIndex());
                        m_lineEdit->setURL(QUrl());
                    }
                    return true;
                }
                case Qt::Key_Return:
                case Qt::Key_Enter:
                {
                    if (currentIndex.isValid())
                    {
                        hide();
                        emit urlChosen(QUrl(currentIndex.data(URLSuggestionListModel::Link).toString()));
                        return true;
                    }
                    return false;
                }
                default:
                    return false;
            }
            return false;
        }
        case QEvent::MouseButtonPress:
        {
            if (!underMouse())
            {
                hide();
                return true;
            }
            return false;
        }
        default:
            return false;
    }
    return false;
}

void URLSuggestionWidget::alignAndShow(const QPoint &urlBarPos, const QRect &urlBarGeom)
{
    QPoint pos;
    pos.setX(urlBarPos.x());
    pos.setY(urlBarPos.y() + urlBarGeom.height());
    move(pos);

    show();
}

void URLSuggestionWidget::suggestForInput(const QString &text)
{
    if (text.isEmpty())
    {
        hide();
        m_model->setSuggestions(std::vector<URLSuggestion>());
        return;
    }
    m_worker->findSuggestionsFor(text);

    /*FaviconStorage *faviconStore = sBrowserApplication->getFaviconStorage();

    std::vector<URLSuggestion> mockData;

    URLSuggestion startPage;
    startPage.Title = QLatin1String("Start Page");
    startPage.URL = QLatin1String("https://www.startpage.com");
    startPage.Favicon = faviconStore->getFavicon(QUrl(startPage.URL));

    URLSuggestion longSearch;
    longSearch.Title = QLatin1String("Searching for something with a long text");
    longSearch.URL = QLatin1String("https://www.google.com/?q=searching+for+something+with+a+long+text");
    longSearch.Favicon = faviconStore->getFavicon(QUrl(longSearch.URL));

    mockData.push_back(startPage);
    mockData.push_back(longSearch);

    m_model->setSuggestions(mockData);*/
}

void URLSuggestionWidget::setURLLineEdit(URLLineEdit *lineEdit)
{
    m_lineEdit = lineEdit;
    m_lineEdit->installEventFilter(this);
}

QSize URLSuggestionWidget::sizeHint() const
{
    QSize widgetSize;
    widgetSize.setWidth(m_lineEdit->width());
    widgetSize.setHeight(m_lineEdit->window()->height() / 6);
    return widgetSize;
}

void URLSuggestionWidget::needResizeWidth(int width)
{
    QTimer::singleShot(150, [=](){ setMinimumWidth(width); });
}
