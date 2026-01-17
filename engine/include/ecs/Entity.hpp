#ifndef VKRT_ENTITY_HPP
#define VKRT_ENTITY_HPP

#include <typeindex>
#include <memory>
#include <vector>

#include "Component.hpp"

namespace vkrt {

/**
 * @class Entity
 * @brief 
 * 
 * 
 */
class Entity {
public:

    template<typename T>
    T* getComponent() {
        for(std::unique_ptr<Component>& component : componentList) {
            if(T* result = dynamic_cast<T*>(component.get())) {
                return result;
            }
        }
        return nullptr;
    }

    std::vector<std::unique_ptr<Component>> componentList;

private:

};

} // namespace vkrt

#endif // VKRT_ENTITY_HPP