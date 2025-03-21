#include "text_buffer.hpp"
#include <fstream>
#include <glm/matrix.hpp>
#include <iostream>
#include <iterator>
#include <regex>

bool LineTextBuffer::load_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << file_path << "\n";
        return false;
    }

    lines.clear();
    std::string line;
    // std::getline is used to read a line of text from an input stream (such as std::cin) into a
    // string. It reads characters until a newline character ('\n') is encountered, at which point it stops reading and
    // stores the characters in the provided string. The newline character is not stored.
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    current_file_path = file_path;
    edit_signal.toggle_state();
    file.close();
    return true;
}

bool LineTextBuffer::save_file() {
    if (current_file_path.empty()) {
        std::cerr << "Error: No file currently loaded.\n";
        return false;
    }

    std::ofstream file(current_file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << current_file_path << " for writing.\n";
        return false;
    }

    for (const auto &line : lines) {
        file << line << "\n";
    }

    file.close();
    modified_without_save = false;
    return true;
}

std::string LineTextBuffer::get_text() const {
    std::string result;
    for (const auto &line : lines) {
        result += line + "\n";
    }
    return result;
}

int LineTextBuffer::line_count() const { return lines.size(); }

std::string LineTextBuffer::get_line(int line_index) const {
    if (line_index < lines.size()) {
        return lines[line_index];
    }
    return "";
}

std::string LineTextBuffer::get_text_from_range(const TextRange &range) const {
    std::string result;
    int start_line = std::clamp(range.start_line, 0, static_cast<int>(lines.size()) - 1);
    int end_line = std::clamp(range.end_line, 0, static_cast<int>(lines.size()) - 1);

    for (int line = start_line; line <= end_line; ++line) {
        int line_length = static_cast<int>(lines[line].size());
        int start_col = (line == range.start_line) ? std::clamp(range.start_col, 0, line_length) : 0;
        int end_col = (line == range.end_line) ? std::clamp(range.end_col, 0, line_length) : line_length;

        // NOTE: because of the above two lines, this is almost always true unless
        // a line has no content because then line_length = 0 and this will be false
        // or the text range is messed up because it is all in one line but for some reason
        // the start col is >= the end col, so there would be no string produced
        if (start_col < end_col) {
            result += lines[line].substr(start_col, end_col - start_col);
        }
        // whenever we iterate over a line we add a newline unless we're on the last iteration
        if (line < end_line) {
            result += "\n";
        }
    }
    return result;
}

TextModification LineTextBuffer::delete_character(int line_index, int col_index) {
    if (line_index >= lines.size() or col_index >= lines[line_index].size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return EMPTY_TEXT_DIFF;
    }

    char deleted_char = lines[line_index][col_index];
    lines[line_index].erase(col_index, 1);

    auto tr = TextRange(line_index, col_index, line_index, col_index + 1);
    auto td = TextModification(tr, "", get_text_from_range(tr));

    edit_signal.toggle_state();
    modified_without_save = true;
    return td;
}

TextModification LineTextBuffer::insert_character(int line_index, int col_index, char character) {
    // Ensure the line_index is within bounds, adding new lines if necessary
    if (line_index >= lines.size()) {
        lines.resize(line_index + 1, std::string()); // Adds empty lines up to line_index
    }

    // Ensure the col_index is within bounds, adding spaces if necessary
    if (col_index > lines[line_index].size()) {
        lines[line_index].resize(col_index, ' '); // Resizes with spaces to the desired column index
    }

    // Insert the character at the specified column index
    lines[line_index].insert(col_index, 1, character);

    // TODO: if we add new lines and stuff do we need to register those diffs as well?
    // yes of course, do this later on.

    auto tm = create_insertion_text_modification(line_index, col_index, std::string(1, character));

    undo_stack.push(tm);

    edit_signal.toggle_state();
    modified_without_save = true;

    return tm;
}

TextModification LineTextBuffer::insert_string(int line_index, int col_index, const std::string &str) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return EMPTY_TEXT_DIFF; // Return an empty diff in case of error
    }

    std::string new_content = str;

    if (col_index > lines[line_index].size()) {
        // If col_index is larger than the line size, resize the line with spaces
        size_t original_size = lines[line_index].size();
        lines[line_index].resize(col_index, ' ');

        new_content = lines[line_index].substr(original_size, col_index - original_size) + str;
    }

    lines[line_index].insert(col_index, str);

    TextRange range(line_index, col_index, line_index, col_index + str.size());
    TextModification modification(range, new_content, "");

    undo_stack.push(modification);
    edit_signal.toggle_state();
    modified_without_save = true;

    return modification;
}

TextModification LineTextBuffer::delete_line(int line_index) {

    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return EMPTY_TEXT_DIFF;
    }

    std::string content_of_line_to_delete_with_newline = lines[line_index] + "\n";

    TextRange range(line_index, 0, line_index + 1, 0);
    TextModification modification(range, "", content_of_line_to_delete_with_newline);

    return apply_text_modification(modification);
}

TextModification LineTextBuffer::append_line(const std::string &line) {
    size_t line_index = lines.size(); // The index for the new line being appended

    lines.push_back(line);

    TextRange range(line_index, 0, line_index, line.size());

    TextModification modification(range, line, ""); // The new content is the line, replaced content is empty

    undo_stack.push(modification);

    edit_signal.toggle_state();
    modified_without_save = true;

    return modification;
}

TextModification LineTextBuffer::replace_line(int line_index, const std::string &new_content) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return EMPTY_TEXT_DIFF;
    }

    std::string old_line_content = lines[line_index];
    lines[line_index] = new_content;

    TextRange range(line_index, 0, line_index, old_line_content.size());

    TextModification modification(range, new_content, old_line_content);

    undo_stack.push(modification);

    edit_signal.toggle_state();
    modified_without_save = true;

    return modification;
}

TextModification LineTextBuffer::insert_newline_after_this_line(int line_index) {

    if (line_index < 0 || line_index > line_count()) {
        return EMPTY_TEXT_DIFF;
    }

    return apply_text_modification(create_newline_diff(line_index + 1));
}

std::string LineTextBuffer::get_bounding_box_string(int start_line, int start_col, int end_line, int end_col) const {
    if (start_line < 0 || start_line >= line_count() || end_line < 0 || end_line >= line_count()) {
        return "";
    }

    int min_line = std::min(start_line, end_line);
    int max_line = std::max(start_line, end_line);
    int min_col = std::min(start_col, end_col);
    int max_col = std::max(start_col, end_col);

    std::string result;

    if (min_line == max_line) {
        result = lines[min_line].substr(min_col, max_col - min_col + 1);
    } else {
        for (int line = min_line; line <= max_line; ++line) {
            if (line == min_line) {
                result += lines[line].substr(min_col) + "\n";
            } else if (line == max_line) {
                result += lines[line].substr(0, max_col + 1);
            } else {
                result += lines[line] + "\n";
            }
        }
    }

    return result;
}

std::vector<TextModification> LineTextBuffer::delete_bounding_box(int start_line, int start_col, int end_line,
                                                                  int end_col) {

    // TODO: think if we should clamp or do this in the future.
    if (start_line < 0 || start_line >= line_count() || end_line < 0 || end_line >= line_count()) {
        return {EMPTY_TEXT_DIFF};
    }

    int min_line = std::min(start_line, end_line);
    int max_line = std::max(start_line, end_line);
    int min_col = std::min(start_col, end_col);
    int max_col = std::max(start_col, end_col);

    std::vector<TextRange> single_line_deletion_ranges;
    for (int line = min_line; line <= max_line; line++) {
        single_line_deletion_ranges.emplace_back(line, min_col, line, max_col);
    }

    std::vector<TextModification> diffs;
    for (const auto &range : single_line_deletion_ranges) {
        std::string part_to_replace =
            lines[range.start_line].substr(range.start_col, range.end_col - range.start_col + 1);
        diffs.emplace_back(range, "", part_to_replace);
        lines[range.start_line].erase(range.start_col, range.end_col - range.start_col + 1); // Perform the deletion
    }

    for (const auto &diff : diffs) {
        undo_stack.push(diff);
    }

    edit_signal.toggle_state();
    modified_without_save = true;

    return diffs;
}

TextModification LineTextBuffer::insert_tab(int line_index, int col_index) {

    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return EMPTY_TEXT_DIFF;
    }

    std::string padding;
    if (col_index > lines[line_index].size()) {
        // Record padding if line is resized
        padding = std::string(col_index - lines[line_index].size(), ' ');
        lines[line_index].resize(col_index, ' ');
    }

    lines[line_index].insert(col_index, TAB);

    std::string modification = padding + TAB;
    size_t start_col = lines[line_index].size() - padding.size();
    size_t end_col = start_col + modification.size();

    auto td = create_insertion_text_modification(line_index, col_index, modification);
    undo_stack.push(td);

    edit_signal.toggle_state();
    modified_without_save = true;

    return td;
}

std::string LineTextBuffer::get_last_deleted_content() const {

    if (undo_stack.empty()) {
        return "";
    }

    const auto &last_diff = undo_stack.top();

    // TODO: handle multiline:
    return last_diff.replaced_content;

    return ""; // No deletion found
}

// TODO return a modification as well.
// TODO: was working on this trying to get the "undo" of newline working.
TextModification LineTextBuffer::apply_text_modification(const TextModification &modification) {
    std::cout << "=== Starting apply_text_modification ===" << std::endl;

    const auto &range = modification.text_range_to_replace;
    const std::string &new_content = modification.new_content;
    const std::string &replaced_content = modification.replaced_content;

    std::cout << "applying: " << modification << std::endl;

    // exactly one of the following if blocks get run
    if (is_newline_deletion(modification)) {
        lines.erase(lines.begin() + range.start_line);
    } else if (is_newline_insertion(modification)) {
        lines.insert(lines.begin() + range.start_line, "");
    } else if (range.start_line == range.end_line) {
        auto &line = lines[range.start_line];

        line.replace(range.start_col, range.end_col - range.start_col, new_content);
        std::cout << "After modification, line: " << line << std::endl;

        // If the replaced content contains a newline, we need to handle the merging
        /*if (new_content.find('\n') != std::string::npos) {*/
        /*    size_t newline_pos = new_content.find('\n');*/
        /*    std::cout << "New content contains newline at position " << newline_pos << std::endl;*/
        /**/
        /*    auto &next_line = lines[range.start_line + 1];*/
        /*    next_line = new_content.substr(newline_pos + 1) + next_line;*/
        /*    line = new_content.substr(0, newline_pos);*/
        /**/
        /*    std::cout << "After splitting, line: " << line << std::endl;*/
        /*    std::cout << "Next line: " << next_line << std::endl;*/
        /*}*/
    } else { // multi-line case
        std::cout << "Modifying multiple lines" << std::endl;

        // Modify the first line from start_col to the end of the line
        auto &first_line = lines[range.start_line];
        std::cout << "Before modification, first line: " << first_line << std::endl;

        first_line.replace(range.start_col, first_line.length() - range.start_col,
                           new_content.substr(0, first_line.length() - range.start_col));
        std::cout << "After modification, first line: " << first_line << std::endl;

        // Modify the intermediate lines entirely, if any
        for (int line_index = range.start_line + 1; line_index < range.end_line; ++line_index) {
            lines[line_index] = new_content.substr(first_line.length() - range.start_col +
                                                       (line_index - range.start_line - 1) * first_line.length(),
                                                   first_line.length());
            std::cout << "After modification, line " << line_index << ": " << lines[line_index] << std::endl;
        }

        // Modify the last line from the start of the line to end_col
        auto &last_line = lines[range.end_line];
        std::cout << "Before modification, last line: " << last_line << std::endl;

        last_line.replace(0, range.end_col,
                          new_content.substr(new_content.length() - last_line.length(), range.end_col));
        std::cout << "After modification, last line: " << last_line << std::endl;

        // Check if new_content contains a newline
        if (new_content.find('\n') != std::string::npos) {
            size_t newline_pos = new_content.find('\n');
            std::cout << "New content contains newline at position " << newline_pos << std::endl;

            auto &next_line = lines[range.end_line + 1];
            next_line = new_content.substr(newline_pos + 1) + next_line;
            lines[range.end_line] = new_content.substr(0, newline_pos);

            std::cout << "After splitting, last line: " << lines[range.end_line] << std::endl;
            std::cout << "Next line: " << next_line << std::endl;
        }
    }

    edit_signal.toggle_state();
    modified_without_save = true;
    undo_stack.push(modification);

    std::cout << "=== Finished apply_text_modification ===" << std::endl;
    return modification;
}

TextModification LineTextBuffer::undo() {
    if (undo_stack.empty()) {
        std::cerr << "Undo stack is empty!\n";
        return EMPTY_TEXT_DIFF;
    }

    TextModification last_change = undo_stack.top();
    undo_stack.pop();
    redo_stack.push(last_change);

    std::cout << "orig: " << last_change << std::endl;
    TextModification inverse_diff = get_inverse_modification(last_change);
    std::cout << "inverse diff: " << inverse_diff << std::endl;
    apply_text_modification(inverse_diff);

    edit_signal.toggle_state();
    modified_without_save = true;
    return inverse_diff;
}

TextModification LineTextBuffer::redo() {
    if (redo_stack.empty()) {
        std::cerr << "Redo stack is empty!\n";
        return EMPTY_TEXT_DIFF;
    }

    TextModification last_redo_change = redo_stack.top();
    redo_stack.pop();

    undo_stack.push(last_redo_change);

    apply_text_modification(last_redo_change);

    edit_signal.toggle_state();   // Toggle the edit signal
    modified_without_save = true; // Mark as modified
    return last_redo_change;
}

int LineTextBuffer::find_rightward_index(int line_index, int col_index, char character) const {
    // Look for the character to the right of col_index in the given line
    std::string line = get_line(line_index);
    for (int i = col_index; i < line.length(); ++i) {
        if (line[i] == character) {
            return i; // Return the column index of the found character
        }
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_leftward_index(int line_index, int col_index, char character) const {
    // Look for the character to the left of col_index in the given line
    std::string line = get_line(line_index);
    for (int i = col_index - 1; i >= 0; --i) {
        if (line[i] == character) {
            return i; // Return the column index of the found character
        }
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_rightward_index_before(int line_index, int col_index, char character) const {
    int found_index = find_rightward_index(line_index, col_index, character);
    if (found_index != col_index) {
        return found_index - 1; // If found, return the column index minus 1
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_leftward_index_before(int line_index, int col_index, char character) const {
    int found_index = find_leftward_index(line_index, col_index, character);
    if (found_index != col_index) {
        return found_index + 1; // If found, return the column index plus 1
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_col_idx_of_first_non_whitespace_character_in_line(int line_index) const {
    int col_index = 0;
    if (line_index < lines.size()) {
        const std::string &line = get_line(line_index);
        while (col_index < line.size() && std::isspace(static_cast<unsigned char>(line[col_index]))) {
            ++col_index;
        }
    }
    return col_index;
}

// TODO: implement and use this later instead of doing isalnum stuff.
bool LineTextBuffer::character_is_non_word_character(const std::string &c) const { return true; }

int LineTextBuffer::find_forward_by_word_index(int line_index, int col_index) const {
    if (line_index < lines.size()) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        // Skip alphanumeric characters
        while (col_index < line.size() && std::isalnum(line[col_index])) {
            ++col_index;
        }

        // Skip non-alphanumeric characters
        while (col_index < line.size() && !std::isalnum(line[col_index])) {
            ++col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_column_index_of_next_character(int line_index, int col_index, char target_char) const {
    bool got_passed_end_of_document = line_index < lines.size();
    if (got_passed_end_of_document) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        // Search for the target character on the current line
        while (col_index < line.size() && line[col_index] != target_char) {
            ++col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_column_index_of_character_leftward(int line_index, int col_index, char target_char) const {
    bool got_passed_end_of_document = line_index < lines.size();
    if (got_passed_end_of_document) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        // Search for the target character on the current line
        while (col_index > 0 && line[col_index] != target_char) {
            --col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_column_index_of_next_right_bracket(int line_index, int col_index) const {
    return find_column_index_of_next_character(line_index, col_index, ')');
}

int LineTextBuffer::find_column_index_of_previous_left_bracket(int line_index, int col_index) const {
    return find_column_index_of_character_leftward(line_index, col_index, '(');
}

int LineTextBuffer::find_forward_to_end_of_word(int line_index, int col_index) const {
    if (line_index < lines.size()) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        // Skip alphanumeric characters if we are not at the beginning of the word
        while (col_index < line.size() && std::isalnum(line[col_index + 1])) {
            ++col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_backward_to_start_of_word(int line_index, int col_index) const {
    if (line_index < lines.size()) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        // Move backwards to skip alphanumeric characters if not at the start of the word
        while (col_index > 0 && std::isalnum(line[col_index + 1])) {
            --col_index;
        }
    }

    return col_index; // Return the updated column index
}

// Updated function to find the previous word by moving the cursor backward
int LineTextBuffer::find_backward_by_word_index(int line_index, int col_index) const {
    if (line_index < lines.size() && col_index >= 0) { // Ensure line_index is valid and col_index is non-negative
        const std::string &line = get_line(line_index);

        // Skip alphanumeric characters backwards
        while (col_index > 0 && std::isalnum(line[col_index - 1])) {
            --col_index;
        }

        // Skip non-alphanumeric characters backwards
        while (col_index > 0 && !std::isalnum(line[col_index - 1])) {
            --col_index;
        }
    }

    return col_index; // Return the updated column index
}

std::string escape_special_chars(const std::string &input) {
    std::string result = input;

    // Escape round, square, and curly braces
    result = std::regex_replace(result, std::regex(R"(\()"), R"(\()");
    result = std::regex_replace(result, std::regex(R"(\))"), R"(\))");
    result = std::regex_replace(result, std::regex(R"(\[)"), R"(\[)");
    result = std::regex_replace(result, std::regex(R"(\])"), R"(\])");
    result = std::regex_replace(result, std::regex(R"(\{)"), R"(\{)");
    result = std::regex_replace(result, std::regex(R"(\})"), R"(\})");

    return result;
}

std::vector<TextRange> LineTextBuffer::find_forward_matches(int line_index, int col_index,
                                                            const std::string &regex_str) const {
    std::vector<TextRange> matches;
    std::string escaped_regex = escape_special_chars(regex_str);
    std::regex pattern(escaped_regex);

    // Check the current line and subsequent lines
    for (int i = line_index; i < lines.size(); ++i) {
        std::string line = lines[i];
        // If it's the starting line, start from col_index, else start from the beginning
        int start_pos = (i == line_index) ? col_index : 0;

        std::smatch match;
        std::string remaining_text = line.substr(start_pos);

        while (std::regex_search(remaining_text, match, pattern)) {
            int start_col = start_pos + match.position(0);
            int end_col = start_pos + match.position(0) + match.length(0);
            matches.push_back(TextRange(i, start_col, i, end_col));
            remaining_text = match.suffix().str();
        }
    }
    return matches;
}

std::vector<TextRange> LineTextBuffer::find_backward_matches(int line_index, int col_index,
                                                             const std::string &regex_str) const {
    std::vector<TextRange> matches;
    std::string escaped_regex = escape_special_chars(regex_str);
    std::regex pattern(escaped_regex);

    // Check the current line and previous lines
    for (int i = line_index; i >= 0; --i) {
        std::string line = lines[i];
        // If it's the starting line, start from col_index, else start from the end of the line
        int start_pos = (i == line_index) ? col_index : line.length();

        std::smatch match;
        std::string remaining_text = line.substr(0, start_pos);

        while (std::regex_search(remaining_text, match, pattern)) {
            int start_col = match.position(0);
            int end_col = match.position(0) + match.length(0);
            matches.push_back(TextRange(i, start_col, i, end_col));
            remaining_text = match.suffix().str();
        }
    }
    return matches;
}

int LineTextBuffer::get_indentation_level(int line, int col) const {
    std::stack<char> bracket_stack; // Stack to keep track of open/close brackets
    int indentation_level = 0;

    // Iterate through all lines up to the specified line
    for (int i = 0; i <= line; ++i) {
        const std::string &current_line = lines[i];

        // Check each character in the line up to the specified column (or end of line)
        for (int j = 0; j < (i == line ? col : current_line.length()); ++j) {
            if (current_line[j] == '{') {
                bracket_stack.push('{'); // Push an open bracket to the stack
                ++indentation_level;     // Increase indentation level for each open bracket
            } else if (current_line[j] == '}') {
                if (!bracket_stack.empty()) {
                    bracket_stack.pop(); // Pop the stack for a matching closing bracket
                    --indentation_level; // Decrease indentation level for each closing bracket
                }
            }
        }
    }

    return indentation_level;
}
