#ifndef DATABASETASKSCHEDULER_H
#define DATABASETASKSCHEDULER_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <QString>

class DatabaseWorker;

/**
 * @class DatabaseWorkerTask
 * @brief Defines the interface for any tasks that will
 *        require access to a \ref DatabaseWorker derived
 *        class
 */
class DatabaseWorkerTask
{
public:
    DatabaseWorkerTask() {}
    virtual ~DatabaseWorkerTask() {}

    /// Runs in the worker thread
    virtual void run(DatabaseWorker *worker) = 0;

    /// Returns the name of the DatabaseWorker that the task requires
    virtual std::string getWorkerName() = 0;
};

/**
 * @class DatabaseTaskScheduler
 * @brief Manages a collection of DatabaseWorkers
 *        that operate outside of the main thread
 */
class DatabaseTaskScheduler
{
public:
    /// Constructs the task schedulerr
    DatabaseTaskScheduler();

    /// Destructor
    ~DatabaseTaskScheduler();

    /// Adds a task to the end of the queue
    void addTask(std::unique_ptr<DatabaseWorkerTask> &&task);

    /// Adds a database worker to the pool of workers. It will be constructed after calling the run() method.
    /// Anything registered with this method after calling run() will not be instantiated
    void addWorker(const std::string &name, std::function<std::unique_ptr<DatabaseWorker>()> construction);

    /// Starts the worker thread
    void run();

private:
    /// Main loop of the worker thread
    void workerThread();

private:
    /// Hashmap of database worker names to their corresponding instances
    std::unordered_map<std::string, std::unique_ptr<DatabaseWorker>> m_registry;

    /// Vector of callbacks waiting to be executed in the run() method
    std::vector<std::pair<std::string, 
        std::function<std::unique_ptr<DatabaseWorker>()>>> m_workersToCreate;

    /// Mutex
    mutable std::mutex m_mutex;

    /// Condition variable
    std::condition_variable m_cv;

    /// Worker thread
    std::thread *m_thread;

    /// Pending tasks
    std::deque<std::unique_ptr<DatabaseWorkerTask>> m_tasks;

    /// Worker flag - when set to false, the worker thread will halt
    bool m_working;
};

#endif // DATABASETASKSCHEDULER_H
