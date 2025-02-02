#include "text_buffer.hpp"
#include <fstream>
#include <iostream>
#include <regex>

bool LineTextBuffer::load_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open file " << file_path << "\n";
        return false;
    }

    lines.clear();
    std::string line;
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

int LineTextBuffer::line_count() const { return lines.size(); }

std::string LineTextBuffer::get_line(int line_index) const {
    if (line_index < lines.size()) {
        return lines[line_index];
    }
    return "";
}

bool LineTextBuffer::delete_character(int line_index, int col_index) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    if (col_index >= lines[line_index].size()) {
        std::cerr << "Error: Column index out of bounds.\n";
        return false;
    }

    char deleted_char = lines[line_index][col_index];
    lines[line_index].erase(col_index, 1);

    // Record the change in the undo stack
    /*undo_stack.push(Diff(Diff::Type::DELETE, line_index, col_index, std::string(1, deleted_char)));*/
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

bool LineTextBuffer::insert_character(int line_index, int col_index, char character) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    if (col_index > lines[line_index].size()) {
        lines[line_index].resize(col_index, ' ');
    }

    lines[line_index].insert(col_index, 1, character);

    // Record the change in the undo stack
    undo_stack.push(Diff(Diff::Type::INSERT, line_index, col_index, std::string(1, character)));
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

bool LineTextBuffer::insert_string(int line_index, int col_index, const std::string &str) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    if (col_index > lines[line_index].size()) {
        // If col_index is greater than the current line size, resize the line with spaces
        lines[line_index].resize(col_index, ' ');
    }

    // Insert the string at the specified position
    lines[line_index].insert(col_index, str);

    // Record the change in the undo stack
    undo_stack.push(Diff(Diff::Type::INSERT, line_index, col_index, str));
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

bool LineTextBuffer::delete_line(int line_index) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }

    std::string deleted_line = lines[line_index];
    lines.erase(lines.begin() + line_index);

    // Record the change in the undo stack
    undo_stack.push(Diff(Diff::Type::DELETE_LINE, line_index, 0, deleted_line));
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

void LineTextBuffer::append_line(const std::string &line) {
    lines.push_back(line);
    undo_stack.push(Diff(Diff::Type::INSERT, lines.size() - 1, 0, line));
    edit_signal.toggle_state();
}

bool LineTextBuffer::replace_line(int line_index, const std::string &new_content) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }

    std::string old_content = lines[line_index];
    lines[line_index] = new_content;

    // Record the change in the undo stack
    undo_stack.push(Diff(Diff::Type::REPLACE, line_index, 0, old_content));
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

bool LineTextBuffer::insert_blank_line(int line_index) {
    if (line_index < 0 || line_index > line_count()) {
        return false;
    }

    lines.insert(lines.begin() + line_index, "");
    undo_stack.push(Diff(Diff::Type::INSERT, line_index, 0, ""));
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
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

bool LineTextBuffer::delete_bounding_box(int start_line, int start_col, int end_line, int end_col) {
    if (start_line < 0 || start_line >= line_count() || end_line < 0 || end_line >= line_count()) {
        return false;
    }

    int min_line = std::min(start_line, end_line);
    int max_line = std::max(start_line, end_line);
    int min_col = std::min(start_col, end_col);
    int max_col = std::max(start_col, end_col);

    std::string deleted_text;

    if (min_line == max_line) {
        deleted_text = lines[min_line].substr(min_col, max_col - min_col + 1);
        lines[min_line].erase(min_col, max_col - min_col + 1);
        undo_stack.push(Diff(Diff::Type::DELETE_WITHIN_LINE, min_line, min_col, deleted_text));
    } else {
        for (int line = min_line; line <= max_line; ++line) {
            if (line == min_line) {
                deleted_text += lines[line].substr(min_col) + "\n";
                lines[line].erase(min_col);
            } else if (line == max_line) {
                deleted_text += lines[line].substr(0, max_col + 1);
                lines[line].erase(0, max_col + 1);
            } else {
                deleted_text += lines[line] + "\n";
                lines[line].clear();
            }
        }
    }

    /*undo_stack.push(Diff(Diff::Type::DELETE, min_line, min_col, deleted_text));*/
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

bool LineTextBuffer::insert_tab(int line_index, int col_index) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    if (col_index > lines[line_index].size()) {
        lines[line_index].resize(col_index, ' ');
    }

    // Insert four spaces at the specified position
    lines[line_index].insert(col_index, "    ");

    // Record the change in the undo stack
    undo_stack.push(Diff(Diff::Type::INSERT, line_index, col_index, "    "));
    edit_signal.toggle_state();
    modified_without_save = true;
    return true;
}

std::string LineTextBuffer::get_last_deleted_content() const {

    if (undo_stack.empty()) {
        return "";
    }

    // Get the most recent Diff object from the undo stack
    const Diff &last_diff = undo_stack.top();

    // If it's a delete operation, return the deleted content
    if (last_diff.type == Diff::Type::DELETE_LINE || last_diff.type == Diff::Type::DELETE_WITHIN_LINE) {
        return last_diff.text;
    }

    return ""; // No deletion found
}

void LineTextBuffer::undo() {
    if (undo_stack.empty()) {
        std::cerr << "Undo stack is empty!\n";
        return;
    }

    Diff last_change = undo_stack.top();
    undo_stack.pop();

    // Revert the last change based on its type
    switch (last_change.type) {
    case Diff::Type::INSERT: {
        delete_character(last_change.line_index, last_change.col_index);
        break;
    }
    case Diff::Type::REPLACE: {
        replace_line(last_change.line_index, last_change.text);
        break;
    }
    case Diff::Type::DELETE_LINE: {
        lines.insert(lines.begin() + last_change.line_index, last_change.text);
        edit_signal.toggle_state();
        break;
    }
    case Diff::Type::DELETE_WITHIN_LINE: {
        lines[last_change.line_index].insert(last_change.col_index, last_change.text);
        break;
    }
    }

    // Push to redo stack
    redo_stack.push(last_change);
}

void LineTextBuffer::redo() {
    if (redo_stack.empty()) {
        std::cerr << "Redo stack is empty!\n";
        return;
    }

    Diff last_change = redo_stack.top();
    redo_stack.pop();

    // Reapply the last undone change based on its type
    switch (last_change.type) {
    case Diff::Type::INSERT: {
        insert_character(last_change.line_index, last_change.col_index, last_change.text[0]);
        break;
    }
    case Diff::Type::REPLACE: {
        replace_line(last_change.line_index, last_change.text);
        break;
    }
    case Diff::Type::DELETE_LINE: {
        delete_line(last_change.line_index);
        break;
    }
    case Diff::Type::DELETE_WITHIN_LINE: {
        delete_bounding_box(last_change.line_index, last_change.col_index, last_change.line_index,
                            last_change.col_index + last_change.text.length() - 1);
        break;
    }
    }

    // Push back to undo stack
    undo_stack.push(last_change);
}

int LineTextBuffer::find_rightward_index(int line_index, int col_index, char character) {
    // Look for the character to the right of col_index in the given line
    std::string line = get_line(line_index);
    for (int i = col_index; i < line.length(); ++i) {
        if (line[i] == character) {
            return i; // Return the column index of the found character
        }
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_leftward_index(int line_index, int col_index, char character) {
    // Look for the character to the left of col_index in the given line
    std::string line = get_line(line_index);
    for (int i = col_index - 1; i >= 0; --i) {
        if (line[i] == character) {
            return i; // Return the column index of the found character
        }
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_rightward_index_before(int line_index, int col_index, char character) {
    int found_index = find_rightward_index(line_index, col_index, character);
    if (found_index != col_index) {
        return found_index - 1; // If found, return the column index minus 1
    }
    return col_index; // If not found, return the original col_index
}

int LineTextBuffer::find_leftward_index_before(int line_index, int col_index, char character) {
    int found_index = find_leftward_index(line_index, col_index, character);
    if (found_index != col_index) {
        return found_index + 1; // If found, return the column index plus 1
    }
    return col_index; // If not found, return the original col_index
}

// Updated function with direct line handling
int LineTextBuffer::find_forward_by_word_index(int line_index, int col_index) {
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

int LineTextBuffer::find_column_index_of_next_right_bracket(int line_index, int col_index) {
    bool got_passed_end_of_document = line_index < lines.size();
    if (got_passed_end_of_document) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        //TODO:this should be generalized to multiple lines 
        while (col_index < line.size() && line[col_index] != ')') {
            ++col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_column_index_of_previous_left_bracket(int line_index, int col_index) {
    bool got_passed_end_of_document = line_index < lines.size();
    if (got_passed_end_of_document) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        //TODO:this should be generalized to multiple lines 
        while (col_index > 0 && line[col_index] != '(') {
            --col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_forward_to_end_of_word(int line_index, int col_index) {
    if (line_index < lines.size()) { // Ensure line_index is valid
        const std::string &line = get_line(line_index);

        // Skip alphanumeric characters if we are not at the beginning of the word
        while (col_index < line.size() && std::isalnum(line[col_index + 1])) {
            ++col_index;
        }
    }

    return col_index; // Return the updated column index
}

int LineTextBuffer::find_backward_to_start_of_word(int line_index, int col_index) {
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
int LineTextBuffer::find_backward_by_word_index(int line_index, int col_index) {
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

std::vector<SubTextIndex> LineTextBuffer::find_forward_matches(int line_index, int col_index,
                                                               const std::string &regex_str) {
    std::vector<SubTextIndex> matches;
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
            matches.push_back(SubTextIndex(i, start_col, i, end_col));
            remaining_text = match.suffix().str();
        }
    }
    return matches;
}

// Backward search function
std::vector<SubTextIndex> LineTextBuffer::find_backward_matches(int line_index, int col_index,
                                                                const std::string &regex_str) {
    std::vector<SubTextIndex> matches;
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
            matches.push_back(SubTextIndex(i, start_col, i, end_col));
            remaining_text = match.suffix().str();
        }
    }
    return matches;
}
