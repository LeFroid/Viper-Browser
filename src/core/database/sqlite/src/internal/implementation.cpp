#include "sqlite3.h"
#include "internal/implementation.h"

#include <chrono>
#include <iostream>
#include <thread>

namespace sqlite
{
namespace internal
{

int busyHandler(void*, int numTries)
{
    // Give up after long enough (5s)
    if (numTries * BusyTimeoutIntervalMs >= 5000)
        return 0;

    std::this_thread::sleep_for(std::chrono::milliseconds{BusyTimeoutIntervalMs});
    return 1;
}

void sqliteLogCallback(void*, int errCode, const char *msg)
{
    std::cerr << "[SQLite3] [" << errCode << "]: " << msg << std::endl;
}

Implementation::Implementation() :
    m_isConfigured{false},
    m_mutex{}
{
}

Implementation &Implementation::instance()
{
    static Implementation impl;
    return impl;
}

void Implementation::init()
{
    std::lock_guard<std::mutex> _{ m_mutex };
    if (!m_isConfigured)
    {
        m_isConfigured = true;
        initBackend();
    }
}

void Implementation::initBackend()
{
    sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
    sqlite3_config(SQLITE_CONFIG_LOG, sqliteLogCallback);
}

}
}
