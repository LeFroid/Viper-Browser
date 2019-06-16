#include "DatabaseFactory.h"
#include "DatabaseTaskScheduler.h"

DatabaseTaskScheduler::DatabaseTaskScheduler() :
    m_registry(),
    m_workersToCreate(),
    m_mutex(),
    m_cv(),
    m_thread(nullptr),
    m_tasks(),
    m_initCallbacks(),
    m_working(false)
{
}

DatabaseTaskScheduler::~DatabaseTaskScheduler()
{
    stop();
}

DatabaseWorker *DatabaseTaskScheduler::getWorker(const std::string &name) const
{
    const auto it = m_registry.find(name);
    if (it != m_registry.end())
        return it->second.get();
    return nullptr;
}

void DatabaseTaskScheduler::onInit(std::function<void()> &&callback)
{
    m_initCallbacks.push_back(std::move(callback));
}

void DatabaseTaskScheduler::post(std::function<void()> &&work)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_tasks.push_back(std::move(work));
    m_cv.notify_one();
}

void DatabaseTaskScheduler::addWorker(const std::string &name, std::function<std::unique_ptr<DatabaseWorker>()> construction)
{
    m_workersToCreate.push_back({name, construction});
}

void DatabaseTaskScheduler::run()
{
    if (m_thread != nullptr || m_working)
        return;

    m_working = true;
    m_thread = new std::thread(&DatabaseTaskScheduler::workerThread, this);
}

void DatabaseTaskScheduler::stop()
{
    if (m_thread != nullptr)
    {
        m_working = false;
        m_cv.notify_all();

        if (m_thread->joinable())
            m_thread->join();

        delete m_thread;
        m_thread = nullptr;
    }
}

void DatabaseTaskScheduler::workerThread()
{
    for (auto &&workerInfo : m_workersToCreate)
    {
        m_registry[workerInfo.first] = workerInfo.second();
    }

    for (auto &initCallback : m_initCallbacks)
        initCallback();

    m_initCallbacks.clear();

    for (;;)
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_cv.wait(lock, [this](){
            return !m_tasks.empty() || !m_working;
        });

        if (!m_working && m_tasks.empty())
            break;

        std::function<void()> task = std::move(m_tasks.front());
        m_tasks.pop_front();
        lock.unlock();

        task();

        if (!m_working && m_tasks.empty())
            break;
    }
}
