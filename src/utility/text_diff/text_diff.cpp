#include "text_diff.hpp"

TextDiff create_insertion_diff(int line, int col, const std::string &text) {
    TextRange range(line, col, line, col);
    return TextDiff(range, text);
}

TextDiff create_newline_diff(int line) {
    TextRange range(line + 1, 0, line + 1, 0);
    return TextDiff(range, "\n");
}

TextDiff create_delete_line_diff(int line) {
    TextRange range(line, 0, line + 1, 0);
    return TextDiff(range, "");
}
