#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include "DatabaseWorker.h"
#include <memory>
#include <type_traits>

/**
 * @class DatabaseFactory
 * @brief Handles the creation of objects whose classes derive from \ref DatabaseWorker
 */
class DatabaseFactory
{
public:
    /// Creates and returns a unique_ptr of an object that inherits the DatabaseWorker class
    template <class Derived>
    static std::unique_ptr<Derived> createWorker(bool firstRun, const QString &databaseFile)
    {
        static_assert(std::is_base_of<DatabaseWorker, Derived>::value, "Object should inherit from DatabaseWorker");
        auto worker = std::make_unique<Derived>(databaseFile);
        if (firstRun)
            worker->setup();

        worker->load();
        return std::move(worker);
    }
};

#endif // DATABASEFACTORY_H
