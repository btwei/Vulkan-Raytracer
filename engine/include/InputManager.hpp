#ifndef VKRT_INPUTMANAGER_HPP
#define VKRT_INPUTMANAGER_HPP

#include "Window.hpp"

namespace vkrt {

enum VKRT_KEYCODE {

};

struct KeyState {
    bool down;
    bool changed;
};

struct MouseState {
    float x;
    float y;
    float dx;
    float dy;
    
    KeyState m1;
    KeyState m2;
    KeyState m3;
};

struct KeyboardState {

};

struct InputState {
    MouseState mouseState;
    KeyboardState keyboardState;
};

class InputManager {
public:
    InputManager(Window* window);

    void init();
    void beginFrame();
    // no special cleanup

    const InputState& getInputState() const { return _inputState; }

private:
    Window* _window;

    InputState _inputState;

    void handleMouseButtonEvent(const SDL_Event& e);

    void updateKeyState(KeyState& current, bool down);
};

} // namespace vkrt

#endif // VKRT_INPUTMANAGER_HPP