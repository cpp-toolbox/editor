#include "input_state.hpp"
#include <GLFW/glfw3.h>
#include <cassert>

#include <iostream>
#include <string>

/* https://toolbox.cuppajoeman.com/ascii_art/keyboard.html
 * ---------------------------------
 * |`|1|2|3|4|5|6|7|8|9|0|-|=|   bs|
 * |tb|q|w|e|r|t|y|u|i|o|p|[|]| \  |
 * |cps|a|s|d|f|g|h|j|k|l|;|'|  ent|
 * |shft|z|x|c|v|b|n|m|,|.|/|  shft|
 * |ct|sp|al|          |al|fn|mn|rc|
 * ---------------------------------
 */

std::vector<std::vector<std::string>> keyboard = {
    {"`", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "   bs"},
    {"tb", "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", "[", "]", " \\  "},
    {"cps", "a", "s", "d", "f", "g", "h", "j", "k", "l", ";", "'", "  ent"},
    {"shft", "z", "x", "c", "v", "b", "n", "m", ",", ".", "/", "  shft"},
    {"ct", "sp", "al", "          ", "al", "fn", "mn", "rc"}};

std::unordered_map<std::string, EKey> keyboard_key_str_to_key_enum = {{"`", EKey::GRAVE_ACCENT},
                                                                      {"1", EKey::ONE},
                                                                      {"2", EKey::TWO},
                                                                      {"3", EKey::THREE},
                                                                      {"4", EKey::FOUR},
                                                                      {"5", EKey::FIVE},
                                                                      {"6", EKey::SIX},
                                                                      {"7", EKey::SEVEN},
                                                                      {"8", EKey::EIGHT},
                                                                      {"9", EKey::NINE},
                                                                      {"0", EKey::ZERO},
                                                                      {"-", EKey::MINUS},
                                                                      {"=", EKey::EQUAL},
                                                                      {"   bs", EKey::BACKSPACE},

                                                                      {"tb", EKey::TAB},
                                                                      {"q", EKey::q},
                                                                      {"w", EKey::w},
                                                                      {"e", EKey::e},
                                                                      {"r", EKey::r},
                                                                      {"t", EKey::t},
                                                                      {"y", EKey::y},
                                                                      {"u", EKey::u},
                                                                      {"i", EKey::i},
                                                                      {"o", EKey::o},
                                                                      {"p", EKey::p},
                                                                      {"[", EKey::LEFT_SQUARE_BRACKET},
                                                                      {"]", EKey::RIGHT_SQUARE_BRACKET},
                                                                      {" \\  ", EKey::BACKSLASH},

                                                                      {"cps", EKey::CAPS_LOCK},
                                                                      {"a", EKey::a},
                                                                      {"s", EKey::s},
                                                                      {"d", EKey::d},
                                                                      {"f", EKey::f},
                                                                      {"g", EKey::g},
                                                                      {"h", EKey::h},
                                                                      {"j", EKey::j},
                                                                      {"k", EKey::k},
                                                                      {"l", EKey::l},
                                                                      {";", EKey::SEMICOLON},
                                                                      {"'", EKey::SINGLE_QUOTE},
                                                                      {"  ent", EKey::ENTER},
                                                                      {"shft", EKey::LEFT_SHIFT},
                                                                      {"z", EKey::z},
                                                                      {"x", EKey::x},
                                                                      {"c", EKey::c},
                                                                      {"v", EKey::v},
                                                                      {"b", EKey::b},
                                                                      {"n", EKey::n},
                                                                      {"m", EKey::m},
                                                                      {",", EKey::COMMA},
                                                                      {".", EKey::PERIOD},
                                                                      {"/", EKey::SLASH},
                                                                      {"  shft", EKey::RIGHT_SHIFT},
                                                                      {"ct", EKey::LEFT_CONTROL},
                                                                      {"sp", EKey::LEFT_SUPER},
                                                                      {"al", EKey::LEFT_ALT},
                                                                      {"          ", EKey::SPACE},
                                                                      {"al", EKey::RIGHT_ALT},
                                                                      {"fn", EKey::FUNCTION_KEY},
                                                                      {"mn", EKey::MENU_KEY},
                                                                      {"rc", EKey::RIGHT_CONTROL}};

std::string InputState::get_string_state() {
    std::stringstream state_stream;

    // Create a copy of the keyboard to modify without changing the original one
    auto modified_keyboard = keyboard;

    // Iterate over all keys in the keyboard
    for (auto &row : modified_keyboard) {
        for (auto &key : row) {
            // Check if the key has a corresponding enum value
            auto key_enum_iter = keyboard_key_str_to_key_enum.find(key);
            if (key_enum_iter != keyboard_key_str_to_key_enum.end()) {
                EKey key_enum = key_enum_iter->second;

                // If the key is just pressed, replace with the correct number of X's
                if (is_just_pressed(key_enum)) {
                    key = std::string(key.size(), '*'); // Replace with X's of the same length
                }
                if (is_pressed(key_enum)) {
                    key = std::string(key.size(), 'X'); // Replace with X's of the same length
                }
            }
        }
    }

    // Construct the state string (instead of printing)
    state_stream << "---------------------------------" << std::endl;
    for (const auto &row : modified_keyboard) {
        for (const auto &key : row) {
            // Ensure alignment of output
            state_stream << "|" << key;
        }
        state_stream << "|" << std::endl;
    }
    state_stream << "---------------------------------" << std::endl;

    // Return the state string
    state_stream << "State updated." << std::endl;

    // Return the constructed state string
    return state_stream.str();
}

bool InputState::is_just_pressed(EKey key_enum) { return key_enum_to_object.at(key_enum)->pressed_signal.is_just_on(); }
bool InputState::is_pressed(EKey key_enum) { return key_enum_to_object.at(key_enum)->pressed_signal.is_on(); }

InputState::InputState() {
    // this line looks trivial but is extremely important, if not the way temporary objects are copied
    // will make it so that the temporal binary states are destroyed making it so that they're not processed anymore
    // horrible bug, so keep this line, using 500 for overkill but I don't like counting.
    this->all_keys.reserve(500);
    all_keys.emplace_back(EKey::a, KeyType::ALPHA, GLFW_KEY_A, "a", false, true, EKey::A);
    all_keys.emplace_back(EKey::b, KeyType::ALPHA, GLFW_KEY_B, "b", false, true, EKey::B);
    all_keys.emplace_back(EKey::c, KeyType::ALPHA, GLFW_KEY_C, "c", false, true, EKey::C);
    all_keys.emplace_back(EKey::d, KeyType::ALPHA, GLFW_KEY_D, "d", false, true, EKey::D);
    all_keys.emplace_back(EKey::e, KeyType::ALPHA, GLFW_KEY_E, "e", false, true, EKey::E);
    all_keys.emplace_back(EKey::f, KeyType::ALPHA, GLFW_KEY_F, "f", false, true, EKey::F);
    all_keys.emplace_back(EKey::g, KeyType::ALPHA, GLFW_KEY_G, "g", false, true, EKey::G);
    all_keys.emplace_back(EKey::h, KeyType::ALPHA, GLFW_KEY_H, "h", false, true, EKey::H);
    all_keys.emplace_back(EKey::i, KeyType::ALPHA, GLFW_KEY_I, "i", false, true, EKey::I);
    all_keys.emplace_back(EKey::j, KeyType::ALPHA, GLFW_KEY_J, "j", false, true, EKey::J);
    all_keys.emplace_back(EKey::k, KeyType::ALPHA, GLFW_KEY_K, "k", false, true, EKey::K);
    all_keys.emplace_back(EKey::l, KeyType::ALPHA, GLFW_KEY_L, "l", false, true, EKey::L);
    all_keys.emplace_back(EKey::m, KeyType::ALPHA, GLFW_KEY_M, "m", false, true, EKey::M);
    all_keys.emplace_back(EKey::n, KeyType::ALPHA, GLFW_KEY_N, "n", false, true, EKey::N);
    all_keys.emplace_back(EKey::o, KeyType::ALPHA, GLFW_KEY_O, "o", false, true, EKey::O);
    all_keys.emplace_back(EKey::p, KeyType::ALPHA, GLFW_KEY_P, "p", false, true, EKey::P);
    all_keys.emplace_back(EKey::q, KeyType::ALPHA, GLFW_KEY_Q, "q", false, true, EKey::Q);
    all_keys.emplace_back(EKey::r, KeyType::ALPHA, GLFW_KEY_R, "r", false, true, EKey::R);
    all_keys.emplace_back(EKey::s, KeyType::ALPHA, GLFW_KEY_S, "s", false, true, EKey::S);
    all_keys.emplace_back(EKey::t, KeyType::ALPHA, GLFW_KEY_T, "t", false, true, EKey::T);
    all_keys.emplace_back(EKey::u, KeyType::ALPHA, GLFW_KEY_U, "u", false, true, EKey::U);
    all_keys.emplace_back(EKey::v, KeyType::ALPHA, GLFW_KEY_V, "v", false, true, EKey::V);
    all_keys.emplace_back(EKey::w, KeyType::ALPHA, GLFW_KEY_W, "w", false, true, EKey::W);
    all_keys.emplace_back(EKey::x, KeyType::ALPHA, GLFW_KEY_X, "x", false, true, EKey::X);
    all_keys.emplace_back(EKey::y, KeyType::ALPHA, GLFW_KEY_Y, "y", false, true, EKey::Y);
    all_keys.emplace_back(EKey::z, KeyType::ALPHA, GLFW_KEY_Z, "z", false, true, EKey::Z);

    all_keys.emplace_back(EKey::A, KeyType::ALPHA, GLFW_KEY_A, "A");
    all_keys.emplace_back(EKey::B, KeyType::ALPHA, GLFW_KEY_B, "B");
    all_keys.emplace_back(EKey::C, KeyType::ALPHA, GLFW_KEY_C, "C");
    all_keys.emplace_back(EKey::D, KeyType::ALPHA, GLFW_KEY_D, "D");
    all_keys.emplace_back(EKey::E, KeyType::ALPHA, GLFW_KEY_E, "E");
    all_keys.emplace_back(EKey::F, KeyType::ALPHA, GLFW_KEY_F, "F");
    all_keys.emplace_back(EKey::G, KeyType::ALPHA, GLFW_KEY_G, "G");
    all_keys.emplace_back(EKey::H, KeyType::ALPHA, GLFW_KEY_H, "H");
    all_keys.emplace_back(EKey::I, KeyType::ALPHA, GLFW_KEY_I, "I");
    all_keys.emplace_back(EKey::J, KeyType::ALPHA, GLFW_KEY_J, "J");
    all_keys.emplace_back(EKey::K, KeyType::ALPHA, GLFW_KEY_K, "K");
    all_keys.emplace_back(EKey::L, KeyType::ALPHA, GLFW_KEY_L, "L");
    all_keys.emplace_back(EKey::M, KeyType::ALPHA, GLFW_KEY_M, "M");
    all_keys.emplace_back(EKey::N, KeyType::ALPHA, GLFW_KEY_N, "N");
    all_keys.emplace_back(EKey::O, KeyType::ALPHA, GLFW_KEY_O, "O");
    all_keys.emplace_back(EKey::P, KeyType::ALPHA, GLFW_KEY_P, "P");
    all_keys.emplace_back(EKey::Q, KeyType::ALPHA, GLFW_KEY_Q, "Q");
    all_keys.emplace_back(EKey::R, KeyType::ALPHA, GLFW_KEY_R, "R");
    all_keys.emplace_back(EKey::S, KeyType::ALPHA, GLFW_KEY_S, "S");
    all_keys.emplace_back(EKey::T, KeyType::ALPHA, GLFW_KEY_T, "T");
    all_keys.emplace_back(EKey::U, KeyType::ALPHA, GLFW_KEY_U, "U");
    all_keys.emplace_back(EKey::V, KeyType::ALPHA, GLFW_KEY_V, "V");
    all_keys.emplace_back(EKey::W, KeyType::ALPHA, GLFW_KEY_W, "W");
    all_keys.emplace_back(EKey::X, KeyType::ALPHA, GLFW_KEY_X, "X");
    all_keys.emplace_back(EKey::Y, KeyType::ALPHA, GLFW_KEY_Y, "Y");
    all_keys.emplace_back(EKey::Z, KeyType::ALPHA, GLFW_KEY_Z, "Z");

    all_keys.emplace_back(EKey::SPACE, KeyType::SYMBOL, GLFW_KEY_SPACE, " ", false);
    all_keys.emplace_back(EKey::GRAVE_ACCENT, KeyType::SYMBOL, GLFW_KEY_GRAVE_ACCENT, "`", false, true, EKey::TILDE);
    all_keys.emplace_back(EKey::TILDE, KeyType::SYMBOL, GLFW_KEY_GRAVE_ACCENT, "`");

    all_keys.emplace_back(EKey::ONE, KeyType::NUMERIC, GLFW_KEY_1, "1", false, true, EKey::EXCLAMATION_POINT);
    all_keys.emplace_back(EKey::TWO, KeyType::NUMERIC, GLFW_KEY_2, "2", false, true, EKey::AT_SIGN);
    all_keys.emplace_back(EKey::THREE, KeyType::NUMERIC, GLFW_KEY_3, "3", false, true, EKey::NUMBER_SIGN);
    all_keys.emplace_back(EKey::FOUR, KeyType::NUMERIC, GLFW_KEY_4, "4", false, true, EKey::DOLLAR_SIGN);
    all_keys.emplace_back(EKey::FIVE, KeyType::NUMERIC, GLFW_KEY_5, "5", false, true, EKey::PERCENT_SIGN);
    all_keys.emplace_back(EKey::SIX, KeyType::NUMERIC, GLFW_KEY_6, "6", false, true, EKey::CARET);
    all_keys.emplace_back(EKey::SEVEN, KeyType::NUMERIC, GLFW_KEY_7, "7", false, true, EKey::AMPERSAND);
    all_keys.emplace_back(EKey::EIGHT, KeyType::NUMERIC, GLFW_KEY_8, "8", false, true, EKey::ASTERISK);
    all_keys.emplace_back(EKey::NINE, KeyType::NUMERIC, GLFW_KEY_9, "9", false, true, EKey::LEFT_PARENTHESIS);
    all_keys.emplace_back(EKey::ZERO, KeyType::NUMERIC, GLFW_KEY_0, "0", false, true, EKey::RIGHT_PARENTHESIS);
    all_keys.emplace_back(EKey::MINUS, KeyType::SYMBOL, GLFW_KEY_MINUS, "-", false, true, EKey::UNDERSCORE);
    all_keys.emplace_back(EKey::EQUAL, KeyType::SYMBOL, GLFW_KEY_EQUAL, "=", false, true, EKey::PLUS);

    all_keys.emplace_back(EKey::EXCLAMATION_POINT, KeyType::SYMBOL, GLFW_KEY_1, "!");
    all_keys.emplace_back(EKey::AT_SIGN, KeyType::SYMBOL, GLFW_KEY_2, "@");
    all_keys.emplace_back(EKey::NUMBER_SIGN, KeyType::SYMBOL, GLFW_KEY_3, "#");
    all_keys.emplace_back(EKey::DOLLAR_SIGN, KeyType::SYMBOL, GLFW_KEY_4, "$");
    all_keys.emplace_back(EKey::PERCENT_SIGN, KeyType::SYMBOL, GLFW_KEY_5, "%");
    all_keys.emplace_back(EKey::CARET, KeyType::SYMBOL, GLFW_KEY_6, "^");
    all_keys.emplace_back(EKey::AMPERSAND, KeyType::SYMBOL, GLFW_KEY_7, "&");
    all_keys.emplace_back(EKey::ASTERISK, KeyType::SYMBOL, GLFW_KEY_8, "*");
    all_keys.emplace_back(EKey::LEFT_PARENTHESIS, KeyType::SYMBOL, GLFW_KEY_9, "(");
    all_keys.emplace_back(EKey::RIGHT_PARENTHESIS, KeyType::SYMBOL, GLFW_KEY_0, ")");
    all_keys.emplace_back(EKey::UNDERSCORE, KeyType::SYMBOL, GLFW_KEY_MINUS, "_");
    all_keys.emplace_back(EKey::PLUS, KeyType::SYMBOL, GLFW_KEY_EQUAL, "+");

    all_keys.emplace_back(EKey::LEFT_SQUARE_BRACKET, KeyType::SYMBOL, GLFW_KEY_LEFT_BRACKET, "[", false, true,
                          EKey::LEFT_CURLY_BRACKET);
    all_keys.emplace_back(EKey::RIGHT_SQUARE_BRACKET, KeyType::SYMBOL, GLFW_KEY_RIGHT_BRACKET, "]", false, true,
                          EKey::RIGHT_CURLY_BRACKET);

    all_keys.emplace_back(EKey::LEFT_CURLY_BRACKET, KeyType::SYMBOL, GLFW_KEY_LEFT_BRACKET, "{");
    all_keys.emplace_back(EKey::RIGHT_CURLY_BRACKET, KeyType::SYMBOL, GLFW_KEY_RIGHT_BRACKET, "}");

    all_keys.emplace_back(EKey::COMMA, KeyType::SYMBOL, GLFW_KEY_COMMA, ",", false, true, EKey::LESS_THAN);
    all_keys.emplace_back(EKey::PERIOD, KeyType::SYMBOL, GLFW_KEY_PERIOD, ".", false, true, EKey::GREATER_THAN);
    all_keys.emplace_back(EKey::LESS_THAN, KeyType::SYMBOL, GLFW_KEY_COMMA, "<");
    all_keys.emplace_back(EKey::GREATER_THAN, KeyType::SYMBOL, GLFW_KEY_PERIOD, ">");

    all_keys.emplace_back(EKey::CAPS_LOCK, KeyType::CONTROL, GLFW_KEY_CAPS_LOCK, "caps_lock", false);
    all_keys.emplace_back(EKey::ESCAPE, KeyType::CONTROL, GLFW_KEY_ESCAPE, "escape", false);
    all_keys.emplace_back(EKey::ENTER, KeyType::CONTROL, GLFW_KEY_ENTER, "enter", false);
    all_keys.emplace_back(EKey::TAB, KeyType::CONTROL, GLFW_KEY_TAB, "tab", false);
    all_keys.emplace_back(EKey::BACKSPACE, KeyType::CONTROL, GLFW_KEY_BACKSPACE, "backspace", false);
    all_keys.emplace_back(EKey::INSERT, KeyType::CONTROL, GLFW_KEY_INSERT, "insert", false);
    all_keys.emplace_back(EKey::DELETE, KeyType::CONTROL, GLFW_KEY_DELETE, "delete", false);

    all_keys.emplace_back(EKey::RIGHT, KeyType::CONTROL, GLFW_KEY_RIGHT, "right", false);
    all_keys.emplace_back(EKey::LEFT, KeyType::CONTROL, GLFW_KEY_LEFT, "left", false);
    all_keys.emplace_back(EKey::UP, KeyType::CONTROL, GLFW_KEY_UP, "up", false);
    all_keys.emplace_back(EKey::DOWN, KeyType::CONTROL, GLFW_KEY_DOWN, "down", false);

    all_keys.emplace_back(EKey::SLASH, KeyType::SYMBOL, GLFW_KEY_SLASH, "/", false, true, EKey::QUESTION_MARK);
    all_keys.emplace_back(EKey::QUESTION_MARK, KeyType::SYMBOL, GLFW_KEY_SLASH, "?");

    all_keys.emplace_back(EKey::BACKSLASH, KeyType::SYMBOL, GLFW_KEY_BACKSLASH, "\\", false, true, EKey::PIPE);
    all_keys.emplace_back(EKey::PIPE, KeyType::SYMBOL, GLFW_KEY_BACKSLASH, "|");

    all_keys.emplace_back(EKey::SEMICOLON, KeyType::SYMBOL, GLFW_KEY_SEMICOLON, ";", false, true, EKey::COLON);
    all_keys.emplace_back(EKey::COLON, KeyType::SYMBOL, GLFW_KEY_SEMICOLON, ":");

    all_keys.emplace_back(EKey::SINGLE_QUOTE, KeyType::SYMBOL, GLFW_KEY_APOSTROPHE, "'", false, true,
                          EKey::DOUBLE_QUOTE);
    all_keys.emplace_back(EKey::DOUBLE_QUOTE, KeyType::SYMBOL, GLFW_KEY_APOSTROPHE, "\"");

    all_keys.emplace_back(EKey::FUNCTION_KEY, KeyType::CONTROL, GLFW_KEY_F1, "function_key", true);
    all_keys.emplace_back(EKey::MENU_KEY, KeyType::CONTROL, GLFW_KEY_MENU, "menu_key", true);

    all_keys.emplace_back(EKey::LEFT_SHIFT, KeyType::MODIFIER, GLFW_KEY_LEFT_SHIFT, "left_shift", false);
    all_keys.emplace_back(EKey::RIGHT_SHIFT, KeyType::MODIFIER, GLFW_KEY_RIGHT_SHIFT, "right_shift", false);
    all_keys.emplace_back(EKey::LEFT_CONTROL, KeyType::MODIFIER, GLFW_KEY_LEFT_CONTROL, "left_control", false);
    all_keys.emplace_back(EKey::RIGHT_CONTROL, KeyType::MODIFIER, GLFW_KEY_RIGHT_CONTROL, "right_control", false);
    all_keys.emplace_back(EKey::LEFT_ALT, KeyType::MODIFIER, GLFW_KEY_LEFT_ALT, "left_alt", false);
    all_keys.emplace_back(EKey::RIGHT_ALT, KeyType::MODIFIER, GLFW_KEY_RIGHT_ALT, "right_alt", false);
    all_keys.emplace_back(EKey::LEFT_SUPER, KeyType::MODIFIER, GLFW_KEY_LEFT_SUPER, "left_super", false);
    all_keys.emplace_back(EKey::RIGHT_SUPER, KeyType::MODIFIER, GLFW_KEY_RIGHT_SUPER, "right_super", false);

    all_keys.emplace_back(EKey::LEFT_MOUSE_BUTTON, KeyType::SYMBOL, GLFW_MOUSE_BUTTON_LEFT, "left_mouse_button", false);
    all_keys.emplace_back(EKey::RIGHT_MOUSE_BUTTON, KeyType::SYMBOL, GLFW_MOUSE_BUTTON_RIGHT, "right_mouse_button",
                          false);
    all_keys.emplace_back(EKey::MIDDLE_MOUSE_BUTTON, KeyType::SYMBOL, GLFW_MOUSE_BUTTON_MIDDLE, "middle_mouse_button",
                          false);
    // not sure how to handle these yet.
    all_keys.emplace_back(EKey::SCROLL_UP, KeyType::SYMBOL, 999, "scroll_up", false);
    /*{EKey::SCROLL_DOWN, KeyType::SYMBOL, 999, "scroll_down"}*/

    for (auto &key : all_keys) {
        // Pointer to the Key object
        Key *key_ptr = &key;

        // Ensure no duplicate key_enum during construction
        assert(this->key_enum_to_object.find(key.key_enum) == this->key_enum_to_object.end() &&
               "Duplicate key_enum detected during construction!");

        // Insert the key_enum to key_object mapping using pointer to store the reference
        this->key_enum_to_object[key.key_enum] = key_ptr;

        if (!key.requires_modifer_to_be_typed) {
            // Ensure no duplicate glfw_code during construction
            assert(this->glfw_code_to_key.find(key.glfw_code) == this->glfw_code_to_key.end() &&
                   "Duplicate glfw_code detected during construction!");

            // Insert the glfw_code to key_object mapping using pointer to store the reference
            this->glfw_code_to_key[key.glfw_code] = key_ptr;
        }
    }
}
