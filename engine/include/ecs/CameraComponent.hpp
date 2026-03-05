#ifndef VKRT_CAMERACOMPONENT_HPP
#define VKRT_CAMERACOMPONENT_HPP

#include <glm/glm.hpp>

#include "Component.hpp"

namespace vkrt {

class CameraComponent : public Component {
public:
    float fov = 90.0f;
    float nearPlane = 0.1f; // Tmin
    float farPlane = 1000.0f; // Tmax

    glm::mat4 viewMatrix = glm::mat4(1.0f); // Updated by the engine Renderer system -- based on transform component
    glm::mat4 projectionMatrix = glm::mat4(1.0f); // Updated by the engine Renderer system -- based on camera component
    bool isDirty = true; // mark dirty when changed, rendering system will mark clean
};

} // namespace vkrt

#endif // VKRT_CAMERACOMPONENT_HPP