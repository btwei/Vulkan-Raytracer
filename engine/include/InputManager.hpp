#ifndef VKRT_INPUTMANAGER_HPP
#define VKRT_INPUTMANAGER_HPP

#include "Window.hpp"

namespace vkrt {

enum VKRT_KEY {
    VKRT_ESCAPE_KEY,
    VKRT_F1_KEY,
    VKRT_F2_KEY,
    VKRT_F3_KEY,
    VKRT_F4_KEY,
    VKRT_F5_KEY,
    VKRT_F6_KEY,
    VKRT_F7_KEY,
    VKRT_F8_KEY,
    VKRT_F9_KEY,
    VKRT_F10_KEY,
    VKRT_F11_KEY,
    VKRT_F12_KEY,
    VKRT_GRAVES_KEY,
    VKRT_1_KEY,
    VKRT_2_KEY,
    VKRT_3_KEY,
    VKRT_4_KEY,
    VKRT_5_KEY,
    VKRT_6_KEY,
    VKRT_7_KEY,
    VKRT_8_KEY,
    VKRT_9_KEY,
    VKRT_0_KEY,
    VKRT_MINUS_KEY,
    VKRT_EQUALS_KEY,
    VKRT_TILDE_KEY,
    VKRT_EXCLAIMATION_POINT_KEY,
    VKRT_AT_SIGN_KEY,
    VKRT_POUND_SIGN_KEY,
    VKRT_DOLLAR_SIGN_KEY,
    VKRY_PERCENT_SIGN_KEY,
    VKRT_UP_CARET_KEY,
    VKRT_AMPERSAND_KEY,
    VKRT_STAR_KEY,
    VKRT_LEFT_PARENTHESIS_KEY,
    VKRT_RIGHT_PARENTHESIS_KEY,
    VKRT_UNDERSCORE_KEY,
    VKRT_PLUS_KEY,
    VKRT_BACKSPACE_KEY,
    VKRT_TAB_KEY,
    VKRT_Q_KEY,
    VKRT_W_KEY,
    VKRT_E_KEY,
    VKRT_R_KEY,
    VKRT_T_KEY,
    VKRT_Y_KEY,
    VKRT_U_KEY,
    VKRT_I_KEY,
    VKRT_O_KEY,
    VKRT_P_KEY,
    VKRT_A_KEY,
    VKRT_S_KEY,
    VKRT_D_KEY,
    VKRT_F_KEY,
    VKRT_G_KEY,
    VKRT_H_KEY,
    VKRT_J_KEY,
    VKRT_K_KEY,
    VKRT_L_KEY,
    VKRT_Z_KEY,
    VKRT_X_KEY,
    VKRT_C_KEY,
    VKRT_V_KEY,
    VKRT_B_KEY,
    VKRT_N_KEY,
    VKRT_M_KEY,
    VKRT_LEFT_CURLY_BRACKET_KEY,
    VKRT_RIGHT_CURLY_BRACKEY_KEY,
    VKRT_VERTICAL_PIPE_KEY,
    VKRT_LEFT_BRACKET_KEY,
    VKRT_RIGHT_BRACKET_KEY,
    VKRT_FORWARD_SLASH_KEY,
    VKRT_SEMICOLON_KEY,
    VKRT_SINGLE_QUOTE_KEY,
    VKRT_COLON_KEY,
    VKRT_DOUBLE_QUOTE_KEY,
    VKRT_COMMA_KEY,
    VKRT_PERIOD_KEY,
    VKRT_BACKSLASH_KEY,
    VKRT_LESS_THAN_KEY,
    VKRT_GREATER_THAN_KEY,
    VKRT_QUESTION_MARK_KEY,
    VKRT_ARROW_LEFT_KEY,
    VKRT_ARROW_UP_KEY,
    VKRT_ARROW_DOWN_KEY,
    VKRT_ARROW_RIGHT_KEY,
    VKRT_KEY_COUNT
};

struct KeyState {
    uint32_t transitionCount = 0;
    bool down = false;
};

struct MouseState {
    float x = 0.0;
    float y = 0.0;
    float dx = 0.0;
    float dy = 0.0;
    
    KeyState m1;
    KeyState m2;
    KeyState m3;
};

struct KeyboardState {
    KeyState keys[VKRT_KEY_COUNT];


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

    void handleKeyboardEvent(const SDL_Event& e);
    void handleMouseMotionEvent(const SDL_Event& e);
    void handleMouseButtonEvent(const SDL_Event& e);

    void updateKeyState(KeyState& current, bool down);
};

} // namespace vkrt

#endif // VKRT_INPUTMANAGER_HPP