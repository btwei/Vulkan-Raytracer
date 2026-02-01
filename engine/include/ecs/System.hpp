#ifndef VKRT_SYSTEM_HPP
#define VKRT_SYSTEM_HPP

#include "Entity.hpp"

namespace vkrt {

class System {
public:
    virtual void update(std::vector<std::unique_ptr<Entity>>& entityList) = 0;
};

} // namespace vkrt

#endif // VKRT_SYSTEM_HPP