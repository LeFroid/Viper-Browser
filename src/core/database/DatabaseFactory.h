#ifndef DATABASEFACTORY_H
#define DATABASEFACTORY_H

#include "DatabaseWorker.h"
#include "ServiceLocator.h"

#include <memory>
#include <type_traits>
#include <QFile>

/**
 * @class DatabaseFactory
 * @brief Handles the creation of objects whose classes derive from \ref DatabaseWorker
 */
class DatabaseFactory
{
public:
    /// Creates and returns a unique_ptr of an object that inherits the DatabaseWorker class
    template <class Derived>
    static std::unique_ptr<DatabaseWorker> createDBWorker(const QString &databaseFile)
    {
        static_assert(std::is_base_of<DatabaseWorker, Derived>::value, "Object should inherit from DatabaseWorker");

        auto worker = std::make_unique<Derived>(databaseFile);
        // Check whether or not a call to DatabaseWorker::setup is needed.
        // If any of the following conditions are met, setup() must be called:
        //    1. Database file is not present on file system
        //    2. Database file exists, but table structure(s) are not present or corrupted
        if (!QFile::exists(databaseFile) || !worker->hasProperStructure())
            worker->setup();

        worker->load();
        return worker;
    }

    /// Creates and returns a unique_ptr of an object that inherits the DatabaseWorker class
    template <class Derived>
    static std::unique_ptr<Derived> createWorker(const QString &databaseFile)
    {
        static_assert(std::is_base_of<DatabaseWorker, Derived>::value, "Object should inherit from DatabaseWorker");

        auto worker = std::make_unique<Derived>(databaseFile);
        // Check whether or not a call to DatabaseWorker::setup is needed.
        // If any of the following conditions are met, setup() must be called:
        //    1. Database file is not present on file system
        //    2. Database file exists, but table structure(s) are not present or corrupted
        if (!QFile::exists(databaseFile) || !worker->hasProperStructure())
            worker->setup();

        worker->load();
        return worker;
    }

    /// Creates and returns a unique_ptr of an object that inherits the DatabaseWorker class
    template <class Derived>
    static std::unique_ptr<Derived> createWorker(const ViperServiceLocator &serviceLocator, const QString &databaseFile)
    {
        static_assert(std::is_base_of<DatabaseWorker, Derived>::value, "Object should inherit from DatabaseWorker");

        auto worker = std::make_unique<Derived>(serviceLocator, databaseFile);
        // Check whether or not a call to DatabaseWorker::setup is needed.
        // If any of the following conditions are met, setup() must be called:
        //    1. Database file is not present on file system
        //    2. Database file exists, but table structure(s) are not present or corrupted
        if (!QFile::exists(databaseFile) || !worker->hasProperStructure())
            worker->setup();

        worker->load();
        return worker;
    }
};

#endif // DATABASEFACTORY_H
