#include "AdBlockLogTableModel.h"

#include <QString>

namespace adblock
{

LogTableModel::LogTableModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_logEntries()
{
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QVariant();

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
            case 0: return tr("Time");
            case 1: return tr("Action");
            case 2: return tr("Rule");
            case 3: return tr("Request Type(s)");
            case 4: return tr("Request URL");
            case 5: return tr("First Party URL");
        }
    }

    return QVariant();
}

int LogTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return static_cast<int>(m_logEntries.size());
}

int LogTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    // Fixed number of columns
    return 6;
}

QVariant LogTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_logEntries.size()))
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    const LogEntry &entry = m_logEntries.at(index.row());
    switch (index.column())
    {
        // Timestamp column
        case 0:
        {
            return entry.Timestamp.toString(QLatin1String("h:mm:ss ap"));
        }
        // Action column
        case 1:
        {
            switch (entry.Action)
            {
                case FilterAction::Allow:
                    return tr("Allow");
                case FilterAction::Block:
                    return tr("Block");
                case FilterAction::Redirect:
                    return tr("Redirected");
            }
            // Will never make it past the above switch
            return tr("?");
        }
        // Rule column
        case 2:
            return entry.Rule;
        // Resource type column
        case 3:
            return elementTypeToString(entry.ResourceType);
        // Request URL column
        case 4:
            return entry.RequestUrl.toString();
        // First party URL column
        case 5:
            return entry.FirstPartyUrl.toString();
    }

    return QVariant();
}

QString LogTableModel::elementTypeToString(ElementType type) const
{
    if (type == ElementType::None)
        return QString();

    auto hasElementType = [](ElementType subject, ElementType target) -> bool {
        return (subject & target) == target;
    };

    std::vector<QString> elemTypes;
    if (hasElementType(type, ElementType::Script))
        elemTypes.push_back(tr("script"));
    if (hasElementType(type, ElementType::Image))
        elemTypes.push_back(tr("image"));
    if (hasElementType(type, ElementType::Stylesheet))
        elemTypes.push_back(tr("stylesheet"));
    if (hasElementType(type, ElementType::Object))
        elemTypes.push_back(tr("object"));
    if (hasElementType(type, ElementType::XMLHTTPRequest))
        elemTypes.push_back(QLatin1String("XHR"));
    if (hasElementType(type, ElementType::ObjectSubrequest))
        elemTypes.push_back(tr("object subrequest"));
    if (hasElementType(type, ElementType::Subdocument))
        elemTypes.push_back(tr("subdocument"));
    if (hasElementType(type, ElementType::Ping))
        elemTypes.push_back(tr("ping"));
    if (hasElementType(type, ElementType::WebSocket))
        elemTypes.push_back(tr("web socket"));
    if (hasElementType(type, ElementType::WebRTC))
        elemTypes.push_back(QLatin1String("RTC"));
    if (hasElementType(type, ElementType::Document))
        elemTypes.push_back(tr("document"));
    if (hasElementType(type, ElementType::ElemHide))
        elemTypes.push_back(tr("elem hide"));
    if (hasElementType(type, ElementType::GenericHide))
        elemTypes.push_back(tr("generic hide"));
    if (hasElementType(type, ElementType::GenericBlock))
        elemTypes.push_back(tr("generic block"));
    if (hasElementType(type, ElementType::PopUp))
        elemTypes.push_back(tr("popup"));
    if (hasElementType(type, ElementType::ThirdParty))
        elemTypes.push_back(tr("third party"));
    if (hasElementType(type, ElementType::MatchCase))
        elemTypes.push_back(tr("match case"));
    if (hasElementType(type, ElementType::Collapse))
        elemTypes.push_back(tr("collapse"));
    if (hasElementType(type, ElementType::BadFilter))
        elemTypes.push_back(QLatin1String("badfilter"));
    if (hasElementType(type, ElementType::CSP))
        elemTypes.push_back(QLatin1String("CSP"));
    if (hasElementType(type, ElementType::InlineScript))
        elemTypes.push_back(tr("inline script"));
    if (hasElementType(type, ElementType::Other))
        elemTypes.push_back(tr("other"));

    QString result;
    for (size_t i = 0; i < elemTypes.size(); ++i)
    {
        result.append(elemTypes[i]);
        if (i + 1 < elemTypes.size())
            result.append(QLatin1String(", "));
    }
    return result;
}

void LogTableModel::setLogEntries(const std::vector<LogEntry> &entries)
{
    beginResetModel();
    m_logEntries = entries;
    endResetModel();
}

}
