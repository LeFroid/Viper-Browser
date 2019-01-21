#ifndef URLSUGGESTIONITEMDELEGATE_H
#define URLSUGGESTIONITEMDELEGATE_H

#include <QStyledItemDelegate>

class URLSuggestionItemDelegate : public QStyledItemDelegate
{
public:
    URLSuggestionItemDelegate(QObject *parent = nullptr);

    /// Renders the delegate using the given painter and style option for the item specified by index.
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    /// Returns the size needed by the delegate to display the item specified by index, taking into account the style information provided by option.
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    /// Padding between items in the paint() method
    int m_padding;
};

#endif // URLSUGGESTIONITEMDELEGATE_H
