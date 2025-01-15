#include "input_state.hpp"

Key get_shifted_key(Key key) {
    switch (key) {
    // Numeric keys to symbols
    case Key::ONE:
        return Key::EXCLAMATION_POINT;
    case Key::TWO:
        return Key::AT_SIGN;
    case Key::THREE:
        return Key::NUMBER_SIGN;
    case Key::FOUR:
        return Key::DOLLAR_SIGN;
    case Key::FIVE:
        return Key::PERCENT_SIGN;
    case Key::SIX:
        return Key::CARET;
    case Key::SEVEN:
        return Key::AMPERSAND;
    case Key::EIGHT:
        return Key::ASTERISK;
    case Key::NINE:
        return Key::LEFT_PARENTHESIS;
    case Key::ZERO:
        return Key::RIGHT_PARENTHESIS;

    // Alphabetic keys to capitalized equivalents
    case Key::A:
        return Key::CAPITAL_A;
    case Key::B:
        return Key::CAPITAL_B;
    case Key::C:
        return Key::CAPITAL_C;
    case Key::D:
        return Key::CAPITAL_D;
    case Key::E:
        return Key::CAPITAL_E;
    case Key::F:
        return Key::CAPITAL_F;
    case Key::G:
        return Key::CAPITAL_G;
    case Key::H:
        return Key::CAPITAL_H;
    case Key::I:
        return Key::CAPITAL_I;
    case Key::J:
        return Key::CAPITAL_J;
    case Key::K:
        return Key::CAPITAL_K;
    case Key::L:
        return Key::CAPITAL_L;
    case Key::M:
        return Key::CAPITAL_M;
    case Key::N:
        return Key::CAPITAL_N;
    case Key::O:
        return Key::CAPITAL_O;
    case Key::P:
        return Key::CAPITAL_P;
    case Key::Q:
        return Key::CAPITAL_Q;
    case Key::R:
        return Key::CAPITAL_R;
    case Key::S:
        return Key::CAPITAL_S;
    case Key::T:
        return Key::CAPITAL_T;
    case Key::U:
        return Key::CAPITAL_U;
    case Key::V:
        return Key::CAPITAL_V;
    case Key::W:
        return Key::CAPITAL_W;
    case Key::X:
        return Key::CAPITAL_X;
    case Key::Y:
        return Key::CAPITAL_Y;
    case Key::Z:
        return Key::CAPITAL_Z;

    default:
        return key; // Return the same key if it's not numeric or alphabetic.
    }
}

KeyType get_key_type(Key key) {
    switch (key) {
    // Alpha-numeric keys
    case Key::A:
    case Key::B:
    case Key::C:
    case Key::D:
    case Key::E:
    case Key::F:
    case Key::G:
    case Key::H:
    case Key::I:
    case Key::J:
    case Key::K:
    case Key::L:
    case Key::M:
    case Key::N:
    case Key::O:
    case Key::P:
    case Key::Q:
    case Key::R:
    case Key::S:
    case Key::T:
    case Key::U:
    case Key::V:
    case Key::W:
    case Key::X:
    case Key::Y:
    case Key::Z:
    case Key::CAPITAL_A:
    case Key::CAPITAL_B:
    case Key::CAPITAL_C:
    case Key::CAPITAL_D:
    case Key::CAPITAL_E:
    case Key::CAPITAL_F:
    case Key::CAPITAL_G:
    case Key::CAPITAL_H:
    case Key::CAPITAL_I:
    case Key::CAPITAL_J:
    case Key::CAPITAL_K:
    case Key::CAPITAL_L:
    case Key::CAPITAL_M:
    case Key::CAPITAL_N:
    case Key::CAPITAL_O:
    case Key::CAPITAL_P:
    case Key::CAPITAL_Q:
    case Key::CAPITAL_R:
    case Key::CAPITAL_S:
    case Key::CAPITAL_T:
    case Key::CAPITAL_U:
    case Key::CAPITAL_V:
    case Key::CAPITAL_W:
    case Key::CAPITAL_X:
    case Key::CAPITAL_Y:
    case Key::CAPITAL_Z:
        return KeyType::ALPHA;

    case Key::ZERO:
    case Key::ONE:
    case Key::TWO:
    case Key::THREE:
    case Key::FOUR:
    case Key::FIVE:
    case Key::SIX:
    case Key::SEVEN:
    case Key::EIGHT:
    case Key::NINE:
        return KeyType::NUMERIC;

    // Modifier keys
    case Key::LEFT_SHIFT:
    case Key::RIGHT_SHIFT:
    case Key::LEFT_CONTROL:
    case Key::RIGHT_CONTROL:
    case Key::LEFT_ALT:
    case Key::RIGHT_ALT:
    case Key::LEFT_SUPER:
    case Key::RIGHT_SUPER:
        return KeyType::MODIFIER;

    // Control keys
    case Key::CAPS_LOCK:
    case Key::ESCAPE:
    case Key::ENTER:
    case Key::TAB:
    case Key::BACKSPACE:
    case Key::INSERT:
    case Key::DELETE:
    case Key::RIGHT:
    case Key::LEFT:
    case Key::UP:
    case Key::DOWN:
    case Key::SLASH:
    case Key::BACKSLASH:
    case Key::COLON:
    case Key::SEMICOLON:
        return KeyType::CONTROL;

    // Control keys
    case Key::EXCLAMATION_POINT:
    case Key::AT_SIGN:
    case Key::NUMBER_SIGN:
    case Key::DOLLAR_SIGN:
    case Key::PERCENT_SIGN:
    case Key::CARET:
    case Key::AMPERSAND:
    case Key::ASTERISK:
    case Key::LEFT_PARENTHESIS:
    case Key::RIGHT_PARENTHESIS:
    case Key::UNDERSCORE:
    case Key::PLUS:
        return KeyType::SYMBOL;

    // Default case to handle invalid or unhandled keys
    default:
        throw std::invalid_argument("Unknown key provided");
    }
}

InputState::InputState() {

    this->key_to_key_string = {{Key::A, "a"},
                               {Key::B, "b"},
                               {Key::C, "c"},
                               {Key::D, "d"},
                               {Key::E, "e"},
                               {Key::F, "f"},
                               {Key::G, "g"},
                               {Key::H, "h"},
                               {Key::I, "i"},
                               {Key::J, "j"},
                               {Key::K, "k"},
                               {Key::L, "l"},
                               {Key::M, "m"},
                               {Key::N, "n"},
                               {Key::O, "o"},
                               {Key::P, "p"},
                               {Key::Q, "q"},
                               {Key::R, "r"},
                               {Key::S, "s"},
                               {Key::T, "t"},
                               {Key::U, "u"},
                               {Key::V, "v"},
                               {Key::W, "w"},
                               {Key::X, "x"},
                               {Key::Y, "y"},
                               {Key::Z, "z"},

                               {Key::CAPITAL_A, "A"},
                               {Key::CAPITAL_B, "B"},
                               {Key::CAPITAL_C, "C"},
                               {Key::CAPITAL_D, "D"},
                               {Key::CAPITAL_E, "E"},
                               {Key::CAPITAL_F, "F"},
                               {Key::CAPITAL_G, "G"},
                               {Key::CAPITAL_H, "H"},
                               {Key::CAPITAL_I, "I"},
                               {Key::CAPITAL_J, "J"},
                               {Key::CAPITAL_K, "K"},
                               {Key::CAPITAL_L, "L"},
                               {Key::CAPITAL_M, "M"},
                               {Key::CAPITAL_N, "N"},
                               {Key::CAPITAL_O, "O"},
                               {Key::CAPITAL_P, "P"},
                               {Key::CAPITAL_Q, "Q"},
                               {Key::CAPITAL_R, "R"},
                               {Key::CAPITAL_S, "S"},
                               {Key::CAPITAL_T, "T"},
                               {Key::CAPITAL_U, "U"},
                               {Key::CAPITAL_V, "V"},
                               {Key::CAPITAL_W, "W"},
                               {Key::CAPITAL_X, "X"},
                               {Key::CAPITAL_Y, "Y"},
                               {Key::CAPITAL_Z, "Z"},

                               {Key::SPACE, "space"},
                               {Key::GRAVE_ACCENT, "~"},

                               {Key::ONE, "1"},
                               {Key::TWO, "2"},
                               {Key::THREE, "3"},
                               {Key::FOUR, "4"},
                               {Key::FIVE, "5"},
                               {Key::SIX, "6"},
                               {Key::SEVEN, "7"},
                               {Key::EIGHT, "8"},
                               {Key::NINE, "9"},
                               {Key::ZERO, "0"},
                               {Key::MINUS, "-"},


                               {Key::EXCLAMATION_POINT, "!"},
                               {Key::AT_SIGN, "@"},
                               {Key::NUMBER_SIGN, "#"},
                               {Key::DOLLAR_SIGN, "$"},
                               {Key::PERCENT_SIGN, "%"},
                               {Key::CARET, "^"},
                               {Key::AMPERSAND, "&"},
                               {Key::ASTERISK, "*"},
                               {Key::LEFT_PARENTHESIS, "("},
                               {Key::RIGHT_PARENTHESIS, ")"},
                               {Key::UNDERSCORE, "_"},
                               {Key::PLUS, "+"},

                               {Key::COMMA, ","},
                               {Key::PERIOD, "."},
                               {Key::PERIOD, "."},
                               {Key::LESS_THAN, "<"},
                               {Key::GREATER_THAN, ">"},

                               {Key::CAPS_LOCK, "caps_lock"},
                               {Key::ESCAPE, "escape"},
                               {Key::ENTER, "enter"},
                               {Key::TAB, "tab"},
                               {Key::BACKSPACE, "backspace"},
                               {Key::INSERT, "insert"},
                               {Key::DELETE, "delete"},

                               {Key::RIGHT, "right"},
                               {Key::LEFT, "left"},
                               {Key::UP, "up"},
                               {Key::DOWN, "down"},

                               {Key::SLASH, "/"},
                               {Key::BACKSLASH, "\\"},
                               {Key::COLON, ":"},
                               {Key::SEMICOLON, ";"},

                               {Key::LEFT_SHIFT, "left_shift"},
                               {Key::RIGHT_SHIFT, "right_shift"},
                               {Key::LEFT_CONTROL, "left_control"},
                               {Key::RIGHT_CONTROL, "right_control"},
                               {Key::LEFT_ALT, "left_alt"},
                               {Key::RIGHT_ALT, "right_alt"},
                               {Key::LEFT_SUPER, "left_super"},
                               {Key::RIGHT_SUPER, "right_super"},

                               {Key::LEFT_MOUSE_BUTTON, "left_mouse_button"},
                               {Key::RIGHT_MOUSE_BUTTON, "right_mouse_button"},
                               {Key::MIDDLE_MOUSE_BUTTON, "middle_mouse_button"},
                               {Key::SCROLL_UP, "scroll_up"},
                               {Key::SCROLL_DOWN, "scroll_down"}};

    this->key_to_glfw_code = {{Key::A, GLFW_KEY_A},
                              {Key::B, GLFW_KEY_B},
                              {Key::C, GLFW_KEY_C},
                              {Key::D, GLFW_KEY_D},
                              {Key::E, GLFW_KEY_E},
                              {Key::F, GLFW_KEY_F},
                              {Key::G, GLFW_KEY_G},
                              {Key::H, GLFW_KEY_H},
                              {Key::I, GLFW_KEY_I},
                              {Key::J, GLFW_KEY_J},
                              {Key::K, GLFW_KEY_K},
                              {Key::L, GLFW_KEY_L},
                              {Key::M, GLFW_KEY_M},
                              {Key::N, GLFW_KEY_N},
                              {Key::O, GLFW_KEY_O},
                              {Key::P, GLFW_KEY_P},
                              {Key::Q, GLFW_KEY_Q},
                              {Key::R, GLFW_KEY_R},
                              {Key::S, GLFW_KEY_S},
                              {Key::T, GLFW_KEY_T},
                              {Key::U, GLFW_KEY_U},
                              {Key::V, GLFW_KEY_V},
                              {Key::W, GLFW_KEY_W},
                              {Key::X, GLFW_KEY_X},
                              {Key::Y, GLFW_KEY_Y},
                              {Key::Z, GLFW_KEY_Z},

                              {Key::SLASH, GLFW_KEY_SLASH},
                              {Key::BACKSLASH, GLFW_KEY_BACKSLASH},

                              {Key::SEMICOLON, GLFW_KEY_SEMICOLON},

                              {Key::SPACE, GLFW_KEY_SPACE},
                              {Key::GRAVE_ACCENT, GLFW_KEY_GRAVE_ACCENT},

                              {Key::ZERO, GLFW_KEY_0},
                              {Key::ONE, GLFW_KEY_1},
                              {Key::TWO, GLFW_KEY_2},
                              {Key::THREE, GLFW_KEY_3},
                              {Key::FOUR, GLFW_KEY_4},
                              {Key::FIVE, GLFW_KEY_5},
                              {Key::SIX, GLFW_KEY_6},
                              {Key::SEVEN, GLFW_KEY_7},
                              {Key::EIGHT, GLFW_KEY_8},
                              {Key::NINE, GLFW_KEY_9},

                              {Key::CAPS_LOCK, GLFW_KEY_CAPS_LOCK},
                              {Key::ESCAPE, GLFW_KEY_ESCAPE},
                              {Key::ENTER, GLFW_KEY_ENTER},
                              {Key::TAB, GLFW_KEY_TAB},
                              {Key::BACKSPACE, GLFW_KEY_BACKSPACE},
                              {Key::INSERT, GLFW_KEY_INSERT},
                              {Key::DELETE, GLFW_KEY_DELETE},

                              {Key::RIGHT, GLFW_KEY_RIGHT},
                              {Key::LEFT, GLFW_KEY_LEFT},
                              {Key::UP, GLFW_KEY_UP},
                              {Key::DOWN, GLFW_KEY_DOWN},

                              {Key::LEFT_SHIFT, GLFW_KEY_LEFT_SHIFT},
                              {Key::RIGHT_SHIFT, GLFW_KEY_RIGHT_SHIFT},
                              {Key::LEFT_CONTROL, GLFW_KEY_LEFT_CONTROL},
                              {Key::RIGHT_CONTROL, GLFW_KEY_RIGHT_CONTROL},

                              {Key::LEFT_ALT, GLFW_KEY_LEFT_ALT},
                              {Key::RIGHT_ALT, GLFW_KEY_RIGHT_ALT},
                              {Key::LEFT_SUPER, GLFW_KEY_LEFT_SUPER},
                              {Key::RIGHT_SUPER, GLFW_KEY_RIGHT_SUPER},
                              {Key::LEFT_MOUSE_BUTTON, GLFW_MOUSE_BUTTON_LEFT},
                              {Key::RIGHT_MOUSE_BUTTON, GLFW_MOUSE_BUTTON_RIGHT},
                              {Key::MIDDLE_MOUSE_BUTTON, GLFW_MOUSE_BUTTON_MIDDLE},
                              // TODO not sure how to handle these yet
                              {Key::SCROLL_UP, 999},
                              {Key::SCROLL_DOWN, 999}};

    // creating a variable by defining a lambda and calling it instantly
    this->key_string_to_key = [this]() {
        std::unordered_map<std::string, Key> map;
        for (const auto &pair : this->key_to_key_string) {
            map[pair.second] = pair.first;
        }
        return map;
    }();

    this->allowed_glfw_codes = [this]() {
        std::unordered_set<int> set;
        for (const auto &pair : this->key_to_glfw_code) {
            set.insert(pair.second);
        }
        return set;
    }();

    this->allowed_key_strings = [this]() {
        std::unordered_set<std::string> set;
        for (const auto &pair : this->key_to_key_string) {
            set.insert(pair.second);
        }
        return set;
    }();

    this->glfw_code_to_key = [this]() {
        std::unordered_map<int, Key> map;
        for (const auto &pair : this->key_to_glfw_code) {
            map[pair.second] = pair.first;
        }
        return map;
    }();

    for (int i = static_cast<int>(Key::A); i < static_cast<int>(Key::DUMMY_COUNT); ++i) {
        Key key = static_cast<Key>(i);
        key_to_state[key]; // default initialization of the temporal binary signal
    }
}
