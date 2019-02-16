#ifndef SERVICELOCATOR_H
#define SERVICELOCATOR_H

#include <string>
#include <unordered_map>

class QObject;

/**
 * @class ServiceLocator
 * @brief Handles the registration and lookup of unique services.
 *        This class does not own any of the services stored in its registry.
 *        For QObject-derived types, this would be instantiated with
 *        KeyType = QString, BaseServiceType = QObject
 */
template <class KeyType, class BaseServiceType>
class ServiceLocator
{
private:
    /// Non-copyable
    ServiceLocator(const ServiceLocator&) = delete;

    /// Non-copyable
    ServiceLocator &operator=(const ServiceLocator&) = delete;

public:
    /// Default constructor
    ServiceLocator() = default;

    /// Default destructor
    ~ServiceLocator() = default;

    /**
     * @brief addService Attempts to add the given service to the registry.
     * @param key Unique identifier of the service
     * @param service Pointer to the service.
     * @return
     */
    bool addService(const KeyType &key, BaseServiceType *service)
    {
        auto it = m_serviceMap.find(key);
        if (it != m_serviceMap.end())
            return false;

        m_serviceMap[key] = service;
        return true;
    }

    /**
     * @brief getService Looks for and attempts to return a service with the given identifier.
     * @param key Unique identifier of the service
     * @return Pointer to the service if found, or a nullptr if not found
     */
    BaseServiceType *getService(const KeyType &key) const
    {
        const auto it = m_serviceMap.find(key);
        if (it == m_serviceMap.end())
            return nullptr;

        return it->second;
    }

    /**
     * @brief getServiceAs Looks for and attempts to return a service with the given identifier.
     * @param key Unique identifier of the service
     * @return Pointer to the service if found, or a nullptr if not found
     */
    template <class Derived>
    Derived *getServiceAs(const KeyType &key) const
    {
        static_assert(std::is_base_of<BaseServiceType, Derived>::value, "Object should inherit from BaseServiceType");

        if (BaseServiceType *service = getService(key))
            return static_cast<Derived*>(service);

        return nullptr;
    }

private:
    std::unordered_map<KeyType, BaseServiceType*> m_serviceMap;
};

using ViperServiceLocator = ServiceLocator<std::string, QObject>;

#endif // SERVICELOCATOR_H
