#ifndef VKRT_GLOBALSINGLETONS_HPP
#define VKRT_GLOBALSINGLETONS_HPP

#include "InputManager.hpp"

namespace vkrt {

/**
 * @brief This struct contains singletons in the ECS system.
 * @note In the future, I may change this to be an unordered_map to allow users to add their own singletons
 */
struct GlobalSingletons {
    InputState inputState;
};

} // namespace vkrt

#endif // VKRT_GLOBALSINGLETONS_HPP