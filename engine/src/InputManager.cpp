#include "InputManager.hpp"

#include <algorithm>

namespace vkrt {

InputManager::InputManager(Window* window) : _window(window) { }

void InputManager::init() {
    _window->registerEventHandler([&, this](SDL_Event e){
        switch(e.type) {
            case SDL_EVENT_KEY_DOWN:
                handleKeyboardEvent(e);
                break;
            case SDL_EVENT_KEY_UP:
                handleKeyboardEvent(e);
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
                handleMouseWheelEvent(e);
                break;
            default:
                // Unhandled SDL event (this is expected for most of the input unrelated events)
        }
    });
}

void InputManager::beginFrame() {
    // Reset mouse transition counts
    _inputState.mouseState.m1.transitionCount = 0;
    _inputState.mouseState.m2.transitionCount = 0;
    _inputState.mouseState.m3.transitionCount = 0;

    // Reset mouse delta values
    _inputState.mouseState.dx = 0;
    _inputState.mouseState.dy = 0;
    _inputState.mouseState.vertScroll = 0;
    _inputState.mouseState.horizontalScroll = 0;

    // Reset keyboard transition counts
    std::for_each(std::begin(_inputState.keyboardState.keys), std::end(_inputState.keyboardState.keys), [](vkrt::KeyState& keyState){
        keyState.transitionCount = 0;
    });
}

void InputManager::handleKeyboardEvent(const SDL_Event& e) {
    switch(e.key.key) {
        case SDLK_ESCAPE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_ESCAPE_KEY], e.key.down); break;
        case SDLK_F1:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F1_KEY], e.key.down); break;
        case SDLK_F2:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F2_KEY], e.key.down); break;
        case SDLK_F3:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F3_KEY], e.key.down); break;
        case SDLK_F4:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F4_KEY], e.key.down); break;
        case SDLK_F5:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F5_KEY], e.key.down); break;
        case SDLK_F6:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F6_KEY], e.key.down); break;
        case SDLK_F7:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F7_KEY], e.key.down); break;
        case SDLK_F8:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F8_KEY], e.key.down); break;
        case SDLK_F9:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F9_KEY], e.key.down); break;
        case SDLK_F10:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F10_KEY], e.key.down); break;
        case SDLK_F11:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F11_KEY], e.key.down); break;
        case SDLK_F12:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F12_KEY], e.key.down); break;
        case SDLK_GRAVE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_GRAVE_KEY], e.key.down); break;
        case SDLK_1:
            updateKeyState(_inputState.keyboardState.keys[VKRT_1_KEY], e.key.down); break;
        case SDLK_2:
            updateKeyState(_inputState.keyboardState.keys[VKRT_2_KEY], e.key.down); break;
        case SDLK_3:
            updateKeyState(_inputState.keyboardState.keys[VKRT_3_KEY], e.key.down); break;
        case SDLK_4:
            updateKeyState(_inputState.keyboardState.keys[VKRT_4_KEY], e.key.down); break;
        case SDLK_5:
            updateKeyState(_inputState.keyboardState.keys[VKRT_5_KEY], e.key.down); break;
        case SDLK_6:
            updateKeyState(_inputState.keyboardState.keys[VKRT_6_KEY], e.key.down); break;
        case SDLK_7:
            updateKeyState(_inputState.keyboardState.keys[VKRT_7_KEY], e.key.down); break;
        case SDLK_8:
            updateKeyState(_inputState.keyboardState.keys[VKRT_8_KEY], e.key.down); break;
        case SDLK_9:
            updateKeyState(_inputState.keyboardState.keys[VKRT_9_KEY], e.key.down); break;
        case SDLK_0:
            updateKeyState(_inputState.keyboardState.keys[VKRT_0_KEY], e.key.down); break;
        case SDLK_MINUS:
            updateKeyState(_inputState.keyboardState.keys[VKRT_MINUS_KEY], e.key.down); break;
        case SDLK_EQUALS:
            updateKeyState(_inputState.keyboardState.keys[VKRT_EQUALS_KEY], e.key.down); break;
        case SDLK_TILDE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_TILDE_KEY], e.key.down); break;
        case SDLK_EXCLAIM:
            updateKeyState(_inputState.keyboardState.keys[VKRT_EXCLAIMATION_POINT_KEY], e.key.down); break;
        case SDLK_AT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_AT_SIGN_KEY], e.key.down); break;
        case SDLK_HASH:
            updateKeyState(_inputState.keyboardState.keys[VKRT_POUND_SIGN_KEY], e.key.down); break;
        case SDLK_DOLLAR:
            updateKeyState(_inputState.keyboardState.keys[VKRT_DOLLAR_SIGN_KEY], e.key.down); break;
        case SDLK_PERCENT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_PERCENT_SIGN_KEY], e.key.down); break;
        case SDLK_CARET:
            updateKeyState(_inputState.keyboardState.keys[VKRT_UP_CARET_KEY], e.key.down); break;
        case SDLK_AMPERSAND:
            updateKeyState(_inputState.keyboardState.keys[VKRT_AMPERSAND_KEY], e.key.down); break;
        case SDLK_ASTERISK:
            updateKeyState(_inputState.keyboardState.keys[VKRT_ASTERISK_KEY], e.key.down); break;
        case SDLK_LEFTPAREN:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LEFT_PARENTHESIS_KEY], e.key.down); break;
        case SDLK_RIGHTPAREN:
            updateKeyState(_inputState.keyboardState.keys[VKRT_RIGHT_PARENTHESIS_KEY], e.key.down); break;
        case SDLK_UNDERSCORE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_UNDERSCORE_KEY], e.key.down); break;
        case SDLK_PLUS:
            updateKeyState(_inputState.keyboardState.keys[VKRT_PLUS_KEY], e.key.down); break;
        case SDLK_BACKSPACE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_BACKSPACE_KEY], e.key.down); break;
        case SDLK_TAB:
            updateKeyState(_inputState.keyboardState.keys[VKRT_TAB_KEY], e.key.down); break;
        case SDLK_CAPSLOCK:
            updateKeyState(_inputState.keyboardState.keys[VKRT_CAPS_LOCK_KEY], e.key.down); break;
        case SDLK_LSHIFT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LSHIFT_KEY], e.key.down); break;
        case SDLK_LCTRL:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LCONTROL_KEY], e.key.down); break;
        case SDLK_LALT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LALT_KEY], e.key.down); break;
        case SDLK_Q:
            updateKeyState(_inputState.keyboardState.keys[VKRT_Q_KEY], e.key.down); break;
        case SDLK_W:
            updateKeyState(_inputState.keyboardState.keys[VKRT_W_KEY], e.key.down); break;
        case SDLK_E:
            updateKeyState(_inputState.keyboardState.keys[VKRT_E_KEY], e.key.down); break;
        case SDLK_R:
            updateKeyState(_inputState.keyboardState.keys[VKRT_R_KEY], e.key.down); break;
        case SDLK_T:
            updateKeyState(_inputState.keyboardState.keys[VKRT_T_KEY], e.key.down); break;
        case SDLK_Y:
            updateKeyState(_inputState.keyboardState.keys[VKRT_Y_KEY], e.key.down); break;
        case SDLK_U:
            updateKeyState(_inputState.keyboardState.keys[VKRT_U_KEY], e.key.down); break;
        case SDLK_I:
            updateKeyState(_inputState.keyboardState.keys[VKRT_I_KEY], e.key.down); break;
        case SDLK_O:
            updateKeyState(_inputState.keyboardState.keys[VKRT_O_KEY], e.key.down); break;
        case SDLK_P:
            updateKeyState(_inputState.keyboardState.keys[VKRT_P_KEY], e.key.down); break;
        case SDLK_A:
            updateKeyState(_inputState.keyboardState.keys[VKRT_A_KEY], e.key.down); break;
        case SDLK_S:
            updateKeyState(_inputState.keyboardState.keys[VKRT_S_KEY], e.key.down); break;
        case SDLK_D:
            updateKeyState(_inputState.keyboardState.keys[VKRT_D_KEY], e.key.down); break;
        case SDLK_F:
            updateKeyState(_inputState.keyboardState.keys[VKRT_F_KEY], e.key.down); break;
        case SDLK_G:
            updateKeyState(_inputState.keyboardState.keys[VKRT_G_KEY], e.key.down); break;
        case SDLK_H:
            updateKeyState(_inputState.keyboardState.keys[VKRT_H_KEY], e.key.down); break;
        case SDLK_J:
            updateKeyState(_inputState.keyboardState.keys[VKRT_J_KEY], e.key.down); break;
        case SDLK_K:
            updateKeyState(_inputState.keyboardState.keys[VKRT_K_KEY], e.key.down); break;
        case SDLK_L:
            updateKeyState(_inputState.keyboardState.keys[VKRT_L_KEY], e.key.down); break;
        case SDLK_Z:
            updateKeyState(_inputState.keyboardState.keys[VKRT_Z_KEY], e.key.down); break;
        case SDLK_X:
            updateKeyState(_inputState.keyboardState.keys[VKRT_X_KEY], e.key.down); break;
        case SDLK_C:
            updateKeyState(_inputState.keyboardState.keys[VKRT_C_KEY], e.key.down); break;
        case SDLK_V:
            updateKeyState(_inputState.keyboardState.keys[VKRT_V_KEY], e.key.down); break;
        case SDLK_B:
            updateKeyState(_inputState.keyboardState.keys[VKRT_B_KEY], e.key.down); break;
        case SDLK_N:
            updateKeyState(_inputState.keyboardState.keys[VKRT_N_KEY], e.key.down); break;
        case SDLK_M:
            updateKeyState(_inputState.keyboardState.keys[VKRT_M_KEY], e.key.down); break;
        case SDLK_LEFTBRACE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LEFT_CURLY_BRACKET_KEY], e.key.down); break;
        case SDLK_RIGHTBRACE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_RIGHT_CURLY_BRACKET_KEY], e.key.down); break;
        case SDLK_PIPE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_VERTICAL_PIPE_KEY], e.key.down); break;
        case SDLK_LEFTBRACKET:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LEFT_BRACKET_KEY], e.key.down); break;
        case SDLK_RIGHTBRACKET:
            updateKeyState(_inputState.keyboardState.keys[VKRT_RIGHT_BRACKET_KEY], e.key.down); break;
        case SDLK_SLASH:
            updateKeyState(_inputState.keyboardState.keys[VKRT_FORWARD_SLASH_KEY], e.key.down); break;
        case SDLK_SEMICOLON:
            updateKeyState(_inputState.keyboardState.keys[VKRT_SEMICOLON_KEY], e.key.down); break;
        case SDLK_APOSTROPHE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_SINGLE_QUOTE_KEY], e.key.down); break;
        case SDLK_COLON:
            updateKeyState(_inputState.keyboardState.keys[VKRT_COLON_KEY], e.key.down); break;
        case SDLK_DBLAPOSTROPHE:
            updateKeyState(_inputState.keyboardState.keys[VKRT_DOUBLE_QUOTE_KEY], e.key.down); break;
        case SDLK_RETURN:
            updateKeyState(_inputState.keyboardState.keys[VKRT_RETURN_KEY], e.key.down); break;
        case SDLK_COMMA:
            updateKeyState(_inputState.keyboardState.keys[VKRT_COMMA_KEY], e.key.down); break;
        case SDLK_PERIOD:
            updateKeyState(_inputState.keyboardState.keys[VKRT_PERIOD_KEY], e.key.down); break;
        case SDLK_BACKSLASH:
            updateKeyState(_inputState.keyboardState.keys[VKRT_BACKSLASH_KEY], e.key.down); break;
        case SDLK_LESS:
            updateKeyState(_inputState.keyboardState.keys[VKRT_LESS_THAN_KEY], e.key.down); break;
        case SDLK_GREATER:
            updateKeyState(_inputState.keyboardState.keys[VKRT_GREATER_THAN_KEY], e.key.down); break;
        case SDLK_QUESTION:
            updateKeyState(_inputState.keyboardState.keys[VKRT_QUESTION_MARK_KEY], e.key.down); break;
        case SDLK_RSHIFT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_RSHIFT_KEY], e.key.down); break;
        case SDLK_LEFT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_ARROW_LEFT_KEY], e.key.down); break;
        case SDLK_UP:
            updateKeyState(_inputState.keyboardState.keys[VKRT_ARROW_UP_KEY], e.key.down); break;
        case SDLK_DOWN:
            updateKeyState(_inputState.keyboardState.keys[VKRT_ARROW_DOWN_KEY], e.key.down); break;
        case SDLK_RIGHT:
            updateKeyState(_inputState.keyboardState.keys[VKRT_ARROW_RIGHT_KEY], e.key.down); break;
        default:
            // Unhandled input; possibly log this in the future
    }
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

void InputManager::handleMouseWheelEvent(const SDL_Event& e) {
    _inputState.mouseState.vertScroll = e.wheel.integer_y;
    _inputState.mouseState.horizontalScroll = e.wheel.integer_x;
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