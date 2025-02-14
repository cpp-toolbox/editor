#ifndef TEXT_DIFF_HPP
#define TEXT_DIFF_HPP

#include <string>

struct TextRange {
    int start_line;
    int start_col;
    int end_line;
    int end_col;

    TextRange(int sl, int sc, int el, int ec) : start_line(sl), start_col(sc), end_line(el), end_col(ec) {}

    bool operator==(const TextRange &other) const {
        return start_line == other.start_line && start_col == other.start_col && end_line == other.end_line &&
               end_col == other.end_col;
    }

    bool operator!=(const TextRange &other) const { return !(*this == other); }
};

// NOTE: suppose we have the following text file:
//
// 0: XXX
// 1: YYY
// 2: ZZZ
//
// to insert a newline, we do the following
//
//   start_line: 3, start_col: 0
//   end_line: 3, end_col: 0
//   replacement_text: "\n"
//
// which gives the following file:
//
// 0: XXX
// 1: YYY
// 2: ZZZ
// 3:
//
// to delete a line we do:
//
//   start_line: 1, start_col: 0
//   end_line: 2, end_col: 0
//   replacement_text: ""
//
// 0: XXX
// 1: ZZZ
// 2:
//
class TextDiff {
  public:
    TextDiff(TextRange text_range, std::string new_content)
        : text_range(std::move(text_range)), new_content(std::move(new_content)) {}

    TextRange text_range;
    std::string new_content;

    bool operator==(const TextDiff &other) const {
        return text_range == other.text_range && new_content == other.new_content;
    }

    bool operator!=(const TextDiff &other) const { return !(*this == other); }
};

TextDiff create_insertion_diff(int line, int col, const std::string &text);
TextDiff create_newline_diff(int line);
TextDiff create_delete_line_diff(int line);

// Constant for an empty text diff
static const TextDiff EMPTY_TEXT_DIFF(TextRange(0, 0, 0, 0), "");

#endif // TEXT_DIFF_HPP
