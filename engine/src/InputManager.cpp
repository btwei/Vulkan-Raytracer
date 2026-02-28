#include "InputManager.hpp"

namespace vkrt {

InputManager::InputManager(Window* window) : _window(window) { }

void InputManager::init() {
    _window->registerEventHandler([&, this](SDL_Event e){
        switch(e.type) {
            case SDL_EVENT_KEY_DOWN:
                break;
            case SDL_EVENT_KEY_UP:
                break;
            case SDL_EVENT_MOUSE_MOTION:
                handleMouseMotionEvent(e);
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                handleMouseButtonEvent(e);
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                handleMouseButtonEvent(e);
                break;
            case SDL_EVENT_MOUSE_WHEEL:

                break;
            default:
        }
    });
}

void InputManager::beginFrame() {
    _inputState.mouseState.m1.transitionCount = 0;
    _inputState.mouseState.m2.transitionCount = 0;
    _inputState.mouseState.m3.transitionCount = 0;
}

void InputManager::handleKeyboardEvent(const SDL_Event& e) {
    //e.
}

void InputManager::handleMouseMotionEvent(const SDL_Event& e) {
    _inputState.mouseState.x = e.motion.x;
    _inputState.mouseState.dx = e.motion.xrel;
    _inputState.mouseState.y = e.motion.y;
    _inputState.mouseState.dy = e.motion.yrel;
}

void InputManager::handleMouseButtonEvent(const SDL_Event& e){
    // Handle SDL mouse button up or down events
    if(e.button.button == SDL_BUTTON_LEFT) {
        updateKeyState(_inputState.mouseState.m1, e.button.down);
    } else if(e.button.button == SDL_BUTTON_RIGHT) {
        updateKeyState(_inputState.mouseState.m2, e.button.down);
    } else if(e.button.button == SDL_BUTTON_MIDDLE) {
        updateKeyState(_inputState.mouseState.m3, e.button.down);
    }
}

void InputManager::updateKeyState(KeyState& state, bool down) {
    // Essentially only track transitions..
    if(down && !state.down) {
        state.down = true;
        state.transitionCount += 1;
    } else if(!down && state.down){
        state.down = false;
        state.transitionCount += 1;
    }
}

} // namespace vkrt