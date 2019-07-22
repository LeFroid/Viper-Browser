#ifndef BROWSERIPC_H
#define BROWSERIPC_H

#include <QSharedMemory>
#include <QSystemSemaphore>

/**
 * @class BrowserIPC
 * @brief The BrowserIPC class is responsible for passing messages
 *        between existing instances of viper-browser. There should
 *        only be one active instance of the application at a time,
 *        so if a second application is spawned to open a URL for example,
 *        it can ask the existing instance to do so in its stead.
 */
class BrowserIPC
{
public:
    /// Size of the shared memory buffer
    static constexpr int BufferLength = 2048;

    /// Constructs the browser IPC instance
    BrowserIPC();

    /// Class destructor
    ~BrowserIPC();

    /// Disable copy constructor
    BrowserIPC(const BrowserIPC &other) = delete;

    /// Disable copy assignment operator
    BrowserIPC &operator=(const BrowserIPC &other) = delete;

    /// Returns true if there is already an instance of the browser application, false otherwise.
    bool hasExistingInstance() const;

    /// Returns true if a message has been posted by another instance of the application.
    /// Otherwise returns false.
    bool hasMessage();

    /// Returns any messages that have been sent to the browser, in the form of a char array
    std::vector<char> getMessage();

    /// Attempts to send the given message (format of data param is length immediately followed by data) through this channel.
    void sendMessage(const char *data, int length);

private:
    /// Releases the shared memory connection. If this is the only application & class
    /// instance connected to the shared memory, it will then be freed.
    void release();

private:
    /// Shared memory segment.
    QSharedMemory m_buffer;

    /// Semaphore for cross-process and thread synchronization
    QSystemSemaphore m_semaphore;

    /// Flag set to true if there is another IPC that owns the shared memory, false if this is the first
    /// instance of the application.
    bool m_hasPreExistingInstance;
};

#endif // BROWSERIPC_H
