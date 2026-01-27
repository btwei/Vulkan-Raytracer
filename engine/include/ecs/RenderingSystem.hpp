#ifndef VKRT_RENDERINGSYSTEM_HPP
#define VKRT_RENDERINGSYSTEM_HPP

#include "System.hpp"

namespace vkrt {

class RenderingSystem : public System {
public:
    virtual void update(std::vector<std::unique_ptr<Entity>>& entityList) override;
};

} // namespace vkrt

#endif // VKRT_RENDERINGSYSTEM_HPP