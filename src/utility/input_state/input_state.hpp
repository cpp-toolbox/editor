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

enum class EKey {
    a,
    b,
    c,
    d,
    e,
    f,
    g,
    h,
    i,
    j,
    k,
    l,
    m,
    n,
    o,
    p,
    q,
    r,
    s,
    t,
    u,
    v,
    w,
    x,
    y,
    z,

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

    SPACE,
    GRAVE_ACCENT,
    TILDE,

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
    EQUAL,

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

    LEFT_SQUARE_BRACKET,
    RIGHT_SQUARE_BRACKET,

    LEFT_CURLY_BRACKET,
    RIGHT_CURLY_BRACKET,

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
    QUESTION_MARK,
    BACKSLASH,
    PIPE,
    COLON,
    SEMICOLON,
    SINGLE_QUOTE,
    DOUBLE_QUOTE,

    LEFT_SHIFT,
    RIGHT_SHIFT,
    LEFT_CONTROL,
    RIGHT_CONTROL,
    LEFT_ALT,
    RIGHT_ALT,
    LEFT_SUPER,
    RIGHT_SUPER,

    FUNCTION_KEY,
    MENU_KEY,

    LEFT_MOUSE_BUTTON,
    RIGHT_MOUSE_BUTTON,
    MIDDLE_MOUSE_BUTTON,
    SCROLL_UP,
    SCROLL_DOWN,

    // the dummy is used as the "biggest value" so that we can iterate through this.
    DUMMY
};

class Key {
  public:
    EKey key_enum;
    KeyType key_type;
    int glfw_code;
    std::string string_repr;
    bool requires_modifer_to_be_typed = true;
    bool shiftable = false;
    EKey key_enum_of_shifted_version = EKey::DUMMY;
    TemporalBinarySignal pressed_signal;
};

// the reason why we have this is so that we can query the entire keyboard and mouse state in a very simple way.
class InputState {
  public:
    InputState();
    ~InputState() = default;

    bool is_just_pressed(EKey key_enum);
    bool is_pressed(EKey key_enum);

    std::string get_string_state();

    std::vector<Key> all_keys;
    std::set<int> glfw_keycodes;
    // pointers to the keys in all_keys
    std::unordered_map<EKey, Key *> key_enum_to_object;
    std::unordered_map<int, Key *> glfw_code_to_key;
};

#endif // INPUT_STATE
