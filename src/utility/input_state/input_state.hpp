#ifndef INPUT_STATE
#define INPUT_STATE

#include <GLFW/glfw3.h>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include "sbpt_generated_includes.hpp"

enum class KeyType {
    ALPHA,
    NUMERIC,
    MODIFIER,
    CONTROL,
    SYMBOL,
};

enum class Key {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    CAPITAL_A,
    CAPITAL_B,
    CAPITAL_C,
    CAPITAL_D,
    CAPITAL_E,
    CAPITAL_F,
    CAPITAL_G,
    CAPITAL_H,
    CAPITAL_I,
    CAPITAL_J,
    CAPITAL_K,
    CAPITAL_L,
    CAPITAL_M,
    CAPITAL_N,
    CAPITAL_O,
    CAPITAL_P,
    CAPITAL_Q,
    CAPITAL_R,
    CAPITAL_S,
    CAPITAL_T,
    CAPITAL_U,
    CAPITAL_V,
    CAPITAL_W,
    CAPITAL_X,
    CAPITAL_Y,
    CAPITAL_Z,

    SPACE,
    GRAVE_ACCENT,

    ONE,
    TWO,
    THREE,
    FOUR,
    FIVE,
    SIX,
    SEVEN,
    EIGHT,
    NINE,
    ZERO,
    MINUS,

    EXCLAMATION_POINT,
    AT_SIGN,
    NUMBER_SIGN,
    DOLLAR_SIGN,
    PERCENT_SIGN,
    CARET,
    AMPERSAND,
    ASTERISK,
    LEFT_PARENTHESIS,
    RIGHT_PARENTHESIS,
    UNDERSCORE,
    PLUS,

    COMMA,
    PERIOD,
    LESS_THAN,
    GREATER_THAN,

    CAPS_LOCK,
    ESCAPE,
    ENTER,
    TAB,
    BACKSPACE,
    INSERT,
    DELETE,

    RIGHT,
    LEFT,
    UP,
    DOWN,

    SLASH,
    BACKSLASH,
    COLON,
    SEMICOLON,

    LEFT_SHIFT,
    RIGHT_SHIFT,
    LEFT_CONTROL,
    RIGHT_CONTROL,
    LEFT_ALT,
    RIGHT_ALT,
    LEFT_SUPER,
    RIGHT_SUPER,

    LEFT_MOUSE_BUTTON,
    RIGHT_MOUSE_BUTTON,
    MIDDLE_MOUSE_BUTTON,
    SCROLL_UP,
    SCROLL_DOWN,

    DUMMY_COUNT
};

Key get_shifted_key(Key key);

KeyType get_key_type(Key key);

// the reason why we have this is so that we can query the entire keyboard and mouse state in a very simple way.
class InputState {
  public:
    InputState();
    ~InputState() = default;

    std::set<int> glfw_keycodes;
    std::unordered_map<Key, TemporalBinarySignal> key_to_state;
    // key maps
    std::unordered_map<Key, std::string> key_to_key_string;
    std::unordered_map<std::string, Key> key_string_to_key;
    std::unordered_map<Key, int> key_to_glfw_code;
    std::unordered_map<int, Key> glfw_code_to_key;
    std::unordered_set<std::string> allowed_key_strings;
    std::unordered_set<int> allowed_glfw_codes;
};

#endif // INPUT_STATE
