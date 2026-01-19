#ifndef VKRT_ENTITY_HPP
#define VKRT_ENTITY_HPP

#include <typeindex>
#include <memory>
#include <vector>

#include "Component.hpp"

namespace vkrt {

/**
 * @file Entity.hpp
 * @brief Contains the Entity class 
 * 
 * This header defines the Entity class for use with the ECS
 * system. Entities are containers that hold a list of components.
 */

/**
 * @class Entity
 * @brief Entity class for use with the ECS system
 * 
 * `Entity` acts as a container for a list of components. The ECS philosophy used
 * is that entities are containers, Components carry state, and Systems provide behavior.
 */
class Entity {
public:
    Entity(const std::string& entityName) : _name(entityName) {}

    const std::string& getName() const { return _name; }
    bool isActive() const { return _active; }
    void setActive(bool isActive) { _active = isActive; }

    template<typename T, typename... Args>
    T* addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component class");
    
        std::unique_ptr<T> component = std::make_unique<T>(std::forward<Args>(args)...);
        T* componentPtr = component.get();
        _componentList.push_back(std::move(component));
        return componentPtr;
    }

    template<typename T>
    T* getComponent() {
        for(std::unique_ptr<Component>& component : _componentList) {
            if(T* result = dynamic_cast<T*>(component.get())) {
                return result;
            }
        }
        return nullptr;
    }

    template<typename T>
    bool removeComponent() {
        for(auto it = _componentList.begin(); it != _componentList.end(); ++it) {
            if(dynamic_cast<T*>(component.get())) {
                _componentList.erase(it);
                return true;
            }
        }
        return false;
    }

private:
    std::string _name;
    bool _active = true;
    std::vector<std::unique_ptr<Component>> _componentList;
};

} // namespace vkrt

#endif // VKRT_ENTITY_HPP