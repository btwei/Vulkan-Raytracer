#include "InputManager.hpp"

namespace vkrt {

InputManager::InputManager(Window* window) : _window(window) { }

void InputManager::init() {
    _window->registerEventHandler([](SDL_Event e){
        
    });
}

} // namespace vkrt