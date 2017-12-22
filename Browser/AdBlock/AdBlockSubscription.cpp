#include "AdBlockSubscription.h"

#include <QFile>
#include <QTextStream>

namespace AdBlock
{
    AdBlockSubscription::AdBlockSubscription() :
        m_enabled(true),
        m_filePath()
    {
    }

    AdBlockSubscription::AdBlockSubscription(const QString &dataFile) :
        m_enabled(true),
        m_filePath(dataFile)
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
}
