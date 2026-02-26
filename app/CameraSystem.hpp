#ifndef CAMERASYSTEM_HPP
#define CAMERASYSTEM_HPP

#include "System.hpp"

/**
 * @class CameraSystem
 * @brief A user defined ECS system that takes user input and updates the active camera's transform
 */
class CameraSystem : public vkrt::System {
public:
    virtual void update(std::vector<std::unique_ptr<vkrt::Entity>>& entityList) override;
};

#endif