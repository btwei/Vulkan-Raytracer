#ifndef VKRT_INPUTMANAGER_HPP
#define VKRT_INPUTMANAGER_HPP

#include "Window.hpp"

namespace vkrt {

class InputManager {
public:
    InputManager(Window* window);

    void init();
    // no update
    // no special cleanup

private:
    Window* _window;
};

} // namespace vkrt

#endif // VKRT_INPUTMANAGER_HPP