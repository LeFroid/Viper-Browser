#ifndef DATABASETASKSCHEDULER_H
#define DATABASETASKSCHEDULER_H

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <QString>

class DatabaseWorker;

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

    /// Returns a database worker that has been registered with the given name
    /// Note: this function should *only* be called in a callback registered with
    /// the onInit() method
    DatabaseWorker *getWorker(const std::string &name) const;

    /// Registers a callback to be executed when the worker thread has started
    void onInit(std::function<void()> &&callback);

    /**
     * @brief Posts a task to the end of the work queue
     * @param f Member function to be invoked
     * @param args Function arguments
     */
    template<class Fn, class ...Args>
    void post(Fn &&f, Args &&...args)
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        m_tasks.push_back(std::bind(std::forward<Fn>(f), std::forward<Args>(args)...));
        m_cv.notify_one();
    }

    /// Posts a task to the end of the work queue
    void post(std::function<void()> &&work);

    /// Adds a database worker to the pool of workers. It will be constructed after calling the run() method.
    /// Anything registered with this method after calling run() will not be instantiated
    void addWorker(const std::string &name, std::function<std::unique_ptr<DatabaseWorker>()> construction);

    /// Starts the worker thread
    void run();

    /// Stops the worker thread
    void stop();

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
    std::deque<std::function<void()>> m_tasks;

    /// Callbacks to be executed after insantiating all of the database workers in the worker thread
    std::vector<std::function<void()>> m_initCallbacks;

    /// Worker flag - when set to false, the worker thread will halt
    bool m_working;
};

#endif // DATABASETASKSCHEDULER_H
