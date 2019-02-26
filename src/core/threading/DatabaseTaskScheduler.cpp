#include "DatabaseFactory.h"
#include "DatabaseTaskScheduler.h"

DatabaseTaskScheduler::DatabaseTaskScheduler() :
    m_registry(),
    m_workersToCreate(),
    m_mutex(),
    m_cv(),
    m_thread(nullptr),
    m_tasks(),
    m_working(false)
{
}

DatabaseTaskScheduler::~DatabaseTaskScheduler()
{
    if (m_thread != nullptr)
    {
        //std::lock_guard<std::mutex> lock{m_mutex};
        m_working = false;
        m_cv.notify_all();

        if (m_thread->joinable())
            m_thread->join();

        delete m_thread;
        m_thread = nullptr;
    }
}

void DatabaseTaskScheduler::addTask(std::unique_ptr<DatabaseWorkerTask> &&task)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_tasks.push_back(std::move(task));
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

void DatabaseTaskScheduler::workerThread()
{
    for (auto &workerInfo : m_workersToCreate)
    {
        m_registry[workerInfo.first] = workerInfo.second();
    }
    m_workersToCreate.clear();

    for (;;)
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        m_cv.wait(lock, [this](){
            return !m_tasks.empty() || !m_working;
        });

        if (!m_working)
            break;

        std::unique_ptr<DatabaseWorkerTask> task = std::move(m_tasks.front());
        m_tasks.pop_front();
        lock.unlock();

        auto it = m_registry.find(task->getWorkerName());
        if (it != m_registry.end())
            task->run(it->second.get());
    }
}
