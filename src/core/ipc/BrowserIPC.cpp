#include "BrowserIPC.h"

#include <QString>
#include <QDebug>

#include <cstring>

BrowserIPC::BrowserIPC() :
    m_buffer(QStringLiteral("_Viper_Browser_IPC_")),
    m_semaphore(QStringLiteral("_Viper_Browser_Sem_"), 1),
    m_hasPreExistingInstance(false)
{
    m_semaphore.acquire();

    // Fix for linux and perhaps other *nix systems (see: https://habr.com/ru/post/173281/)
    {
        QSharedMemory sharedMemTemp(QStringLiteral("_Viper_Browser_IPC_"));
        sharedMemTemp.attach();
    }

    // Attach to or create the shared memory region
    if (m_buffer.attach())
    {
        m_hasPreExistingInstance = true;
    }
    else
    {
        m_buffer.create(BufferLength);
        memset(m_buffer.data(), 0, BufferLength);
        m_hasPreExistingInstance = false;
    }

    m_semaphore.release();
}

BrowserIPC::~BrowserIPC()
{
    release();
}

void BrowserIPC::release()
{
    m_semaphore.acquire();

    if (m_buffer.isAttached())
        m_buffer.detach();

    m_semaphore.release();
}

bool BrowserIPC::hasExistingInstance() const
{
    return m_hasPreExistingInstance;
}

bool BrowserIPC::hasMessage()
{
    if (!m_buffer.isAttached())
        return false;

    m_semaphore.acquire();
    const char *data = static_cast<const char*>(m_buffer.constData());
    m_semaphore.release();

    if (data[sizeof(int)] == '\0')
        return false;

    const int messageLen = *(reinterpret_cast<const int*>(data));
    return messageLen > 0 && messageLen < BufferLength;
}

std::vector<char> BrowserIPC::getMessage()
{
    if (!m_buffer.isAttached())
        return std::vector<char>();

    // Read from buffer
    m_semaphore.acquire();
    const char *data = reinterpret_cast<const char*>(m_buffer.constData());
    int length = m_buffer.size();

    // Sanity checks
    if (length < 1)
    {
        qDebug() << "BrowserIPC::getMessage() - invalid message (length < 1 or data is null)";
        m_semaphore.release();
        return std::vector<char>();
    }

    const int expectedLength = *(reinterpret_cast<const int*>(data));
    if (expectedLength < 1 || expectedLength > BufferLength)
    {
        qDebug() << "BrowserIPC::getMessage() - invalid expected length";
        m_semaphore.release();
        return std::vector<char>();
    }

    // Validate the expected length against the actual
    const int offset = sizeof(int);
    int actualLength = 0;
    while (actualLength < BufferLength)
    {
        if (*(data + offset + actualLength) == '\0')
            break;

        actualLength++;
    }

    if (actualLength != expectedLength)
    {
        qDebug() << "BrowserIPC::getMessage() - actual length: " << actualLength << ", expected: " << expectedLength;
        m_semaphore.release();
        return std::vector<char>();
    }

    // Copy buffer and clear the shared memory region
    std::vector<char> result(static_cast<size_t>(actualLength), '\0');

    char *resultPtr = result.data();
    memcpy(resultPtr, &data[offset], static_cast<size_t>(actualLength));

    // Clear shared memory
    memset(m_buffer.data(), 0, static_cast<size_t>(offset + actualLength));

    m_semaphore.release();

    return result;
}

void BrowserIPC::sendMessage(const char *data, int length)
{
    if (data == nullptr || length < 1 || length > BufferLength)
    {
        qDebug() << "BrowserIPC:: failed to send message";
        return;
    }

    // If there are pending messages, they will be overwritten, but that is acceptable for
    // our use case.
    m_semaphore.acquire();

    char *dest = reinterpret_cast<char*>(m_buffer.data());
    memcpy(dest, data, static_cast<size_t>(length));

    m_semaphore.release();
}
