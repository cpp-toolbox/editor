#ifndef TEXT_DIFF_HPP
#define TEXT_DIFF_HPP

#include <iostream>
#include <string>

// A range in a text document expressed as (zero-based) start and end positions. A range is comparable to a selection in
// an editor. Therefore, the end position is exclusive. If you want to specify a range that contains a line including
// the line ending character(s) then use an end position denoting the start of the next line. For example:
//
// {
// start: {
// line:
//     5, character : 23
// }
//     , end : {
//     line:
//         6, character : 0
//     }
// }
//

// a text range that has the the same start and end place is an empty selection, empty selections can be used
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

    // Define operator<< inside the class
    friend std::ostream &operator<<(std::ostream &os, const TextRange &range) {
        os << "start: (" << range.start_line << ", " << range.start_col << "), "
           << "end: (" << range.end_line << ", " << range.end_col << ")";
        return os;
    }
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

// a text modifiction takes in a range to replace, the content to replace and then the content that used to be there.
// to do an insertion you just pass a range at the right place which is empty.
// note that text modifications should be in the context that a file is a single string with \n for newlines
// even if the way you handle your buffer is different under the hood
class TextModification {
  public:
    TextModification(TextRange text_range_to_replace, const std::string &new_content,
                     const std::string &replaced_content)
        : text_range_to_replace(text_range_to_replace), new_content(new_content), replaced_content(replaced_content) {}

    TextRange text_range_to_replace;
    std::string new_content;
    std::string replaced_content;

    bool operator==(const TextModification &other) const {
        return text_range_to_replace == other.text_range_to_replace && new_content == other.new_content;
    }

    bool operator!=(const TextModification &other) const { return !(*this == other); }

    friend std::ostream &operator<<(std::ostream &os, const TextModification &modification) {
        os << "TextModification {\n"
           << "  text_range: " << modification.text_range_to_replace << "\n"
           << "  new_content: \"" << modification.new_content << "\"\n"
           << "  replaced_content: \"" << modification.replaced_content << "\"\n"
           << "}";
        return os;
    }
};

TextModification get_inverse_modification(const TextModification &modification);

TextModification create_insertion_text_modification(int line, int col, const std::string &text);
TextModification create_newline_diff(int line);

bool is_insertion(const TextModification &modification);
bool is_newline_insertion(const TextModification &modification);
bool is_newline_deletion(const TextModification &modification);

// Constant for an empty text diff
static const TextModification EMPTY_TEXT_DIFF(TextRange(0, 0, 0, 0), "", "");

#endif // TEXT_DIFF_HPP
