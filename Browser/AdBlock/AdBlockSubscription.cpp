#include "AdBlockSubscription.h"

#include <QFile>
#include <QTextStream>

AdBlockSubscription::AdBlockSubscription() :
    m_enabled(true),
    m_filePath(),
    m_sourceUrl(),
    m_lastUpdate(),
    m_nextUpdate(),
    m_filters()
{
}

AdBlockSubscription::AdBlockSubscription(const QString &dataFile) :
    m_enabled(true),
    m_filePath(dataFile),
    m_sourceUrl(),
    m_lastUpdate(),
    m_nextUpdate(),
    m_filters()
{
}

AdBlockSubscription::AdBlockSubscription(AdBlockSubscription &&other) :
    m_enabled(other.m_enabled),
    m_filePath(other.m_filePath),
    m_sourceUrl(other.m_sourceUrl),
    m_lastUpdate(other.m_lastUpdate),
    m_nextUpdate(other.m_nextUpdate),
    m_filters(std::move(other.m_filters))
{
}

AdBlockSubscription &AdBlockSubscription::operator =(AdBlockSubscription &&other)
{
    if (this != &other)
    {
        m_enabled = other.m_enabled;
        m_filePath = other.m_filePath;
        m_sourceUrl = other.m_sourceUrl;
        m_lastUpdate = other.m_lastUpdate;
        m_nextUpdate = other.m_nextUpdate;
        m_filters = std::move(other.m_filters);
    }

    return *this;
}

AdBlockSubscription::~AdBlockSubscription()
{
}

bool AdBlockSubscription::isEnabled() const
{
    return m_enabled;
}

void AdBlockSubscription::setEnabled(bool value)
{
    m_enabled = value;
}

const QUrl &AdBlockSubscription::getSourceUrl() const
{
    return m_sourceUrl;
}

const QDateTime &AdBlockSubscription::getLastUpdate() const
{
    return m_lastUpdate;
}

const QDateTime &AdBlockSubscription::getNextUpdate() const
{
    return m_nextUpdate;
}

void AdBlockSubscription::load()
{
    if (!m_enabled || m_filePath.isEmpty())
        return;

    QFile subFile(m_filePath);
    if (!subFile.exists() || !subFile.open(QIODevice::ReadOnly))
        return;

    QString line;
    QTextStream stream(&subFile);
    while (stream.readLineInto(&line))
    {
        if (line.startsWith(QChar('!')) || line.startsWith("# ") || line.startsWith(QStringLiteral("[Adblock")))
            continue;

        m_filters.push_back(std::make_unique<AdBlockFilter>(line));
    }
}

void AdBlockSubscription::setLastUpdate(const QDateTime &date)
{
    m_lastUpdate = date;
}

void AdBlockSubscription::setNextUpdate(const QDateTime &date)
{
    m_nextUpdate = date;
}

void AdBlockSubscription::setSourceUrl(const QUrl &source)
{
    m_sourceUrl = source;
}

int AdBlockSubscription::getNumFilters() const
{
    return static_cast<int>(m_filters.size());
}

AdBlockFilter *AdBlockSubscription::getFilter(int index)
{
    if (index < 0 || index >= static_cast<int>(m_filters.size()))
        return nullptr;
    return m_filters[index].get();
}

const QString &AdBlockSubscription::getFilePath() const
{
    return m_filePath;
}
