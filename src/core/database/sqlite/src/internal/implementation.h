#ifndef _SQLITE_INTERNAL_IMPLEMENTATION_H_
#define _SQLITE_INTERNAL_IMPLEMENTATION_H_

#include <mutex>

namespace sqlite
{

namespace internal
{

constexpr int BusyTimeoutIntervalMs = 250;

/// Handles a busy error when a database is locked by another thread
int busyHandler(void*, int numTries);

/**
 * @class Implementation
 * @brief Contains internal library settings and state management routines.
 */
class Implementation
{
private:
    /// Default constructor
    Implementation();

public:
    /// Returns the single instance of the implementation class
    static Implementation &instance();

    /// Sets runtime SQLite3 config settings, if not already done so.
    void init();

private:
    /// Sets runtime SQLite3 config settings
    void initBackend();

public:
    /// Forbid copying
    Implementation(const Implementation&) = delete;
    Implementation &operator=(const Implementation&) = delete;

private:
    /// Flag indicating whether or not the library has been configured
    bool m_isConfigured { false };

    /// To ensure the initLibrary() method is only called once in multi-threaded environments
    std::mutex m_mutex;
};

}

}

#endif // _SQLITE_INTERNAL_IMPLEMENTATION_H_

