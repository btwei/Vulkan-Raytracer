#ifndef VKRT_SYSTEM_HPP
#define VKRT_SYSTEM_HPP

#include <functional>
#include <memory>
#include <vector>

#include "Entity.hpp"
#include "GlobalSingletons.hpp"

namespace vkrt {

class System {
public:
    virtual ~System() = default;
    virtual void update(std::vector<std::unique_ptr<Entity>>& entityList, const GlobalSingletons globalSingletons) = 0;
};

} // namespace vkrt

#endif // VKRT_SYSTEM_HPP