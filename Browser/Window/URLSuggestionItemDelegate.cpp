#include "URLSuggestionItemDelegate.h"

URLSuggestionItemDelegate::URLSuggestionItemDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{
}

void URLSuggestionItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem itemOption = option;
    initStyleOption(&itemOption, index);

    // draw favicon

    // draw title

    // draw url under title
}

QSize URLSuggestionItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    /*
    QSize hint;
    return hint;
    */
    return QStyledItemDelegate::sizeHint(option, index);
}
