#include "ITextFinder.h"

ITextFinder::ITextFinder(QObject *parent) :
    QObject(parent),
    m_searchTerm(),
    m_highlightAll(false),
    m_matchCase(false),
    m_numOccurrences(0),
    m_occurrenceCounter(0)
{
}
