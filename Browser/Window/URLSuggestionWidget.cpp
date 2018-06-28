#include "URLSuggestionWidget.h"
#include "URLLineEdit.h"

#include <QListView>
#include <QTimer>
#include <QVBoxLayout>

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
    //m_suggestionList->setItemDelegate(new URLSuggestionItemDelegate(this));

    auto vboxLayout = new QVBoxLayout(this);
    vboxLayout->setSpacing(0);
    vboxLayout->setContentsMargins(0, 0, 0, 0);

    vboxLayout->addWidget(m_suggestionList);
}

void URLSuggestionWidget::alignAndShow(const QPoint &urlBarPos, const QRect &urlBarGeom)
{
    QPoint pos;
    pos.setX(urlBarPos.x());
    pos.setY(urlBarPos.y() + urlBarGeom.height() + 6);
    move(pos);

    // todo: implement some API like the following, which finds urls to add to the model in a separate thread of execution
    // model->suggestForInput(m_lineEdit->text());

    show();
}

void URLSuggestionWidget::setURLLineEdit(URLLineEdit *lineEdit)
{
    m_lineEdit = lineEdit;
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
