#ifndef VKRT_TRANSFORMCOMPONENT_HPP
#define VKRT_TRANSFORMCOMPONENT_HPP

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Component.hpp"

namespace vkrt {

class TransformComponent : public Component {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 cachedTransform = glm::mat4(1.0f);
    bool isDirty = true; // mark dirty when changed, rendering system will mark clean
};

} // namespace vkrt

#endif