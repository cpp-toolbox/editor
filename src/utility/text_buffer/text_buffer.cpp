#include "text_buffer.hpp"

#include <fstream>
#include <iostream>

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

bool LineTextBuffer::save_file() const {
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
    lines[line_index].erase(col_index, 1);
    edit_signal.toggle_state();
    return true;
}

bool LineTextBuffer::insert_character(int line_index, int col_index, char character) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    if (col_index > lines[line_index].size()) {
        // If col_index is greater than the current line size, insert spaces up to col_index
        lines[line_index].resize(col_index, ' ');
    }
    lines[line_index].insert(col_index, 1, character);
    edit_signal.toggle_state();
    return true;
}


bool LineTextBuffer::delete_line(int line_index) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    lines.erase(lines.begin() + line_index);
    edit_signal.toggle_state();
    return true;
}

void LineTextBuffer::append_line(const std::string &line) { lines.push_back(line); }

bool LineTextBuffer::replace_line(int line_index, const std::string &new_content) {
    if (line_index >= lines.size()) {
        std::cerr << "Error: line index out of bounds.\n";
        return false;
    }
    lines[line_index] = new_content;
    edit_signal.toggle_state();
    return true;
}

bool LineTextBuffer::insert_blank_line(int line_index) {
    if (line_index < 0 || line_index > line_count()) {
        // Invalid index, return false
        return false;
    }
    // Insert an empty string at the specified index
    lines.insert(lines.begin() + line_index, "");
    edit_signal.toggle_state();
    return true;
}

bool LineTextBuffer::delete_bounding_box(int start_line, int start_col, int end_line, int end_col) {
    // Ensure the start and end points are within bounds
    if (start_line < 0 || start_line >= line_count() || end_line < 0 || end_line >= line_count()) {
        return false;
    }

    // Calculate the minimum and maximum line and column indices
    int min_line = std::min(start_line, end_line);
    int max_line = std::max(start_line, end_line);
    int min_col = std::min(start_col, end_col);
    int max_col = std::max(start_col, end_col);

    // Ensure column indices are within bounds for each line
    if (min_col < 0 || max_col < 0) {
        return false;
    }

    // Handle the case where the bounding box is confined to a single line
    if (min_line == max_line) {
        if (min_col <= max_col && max_col < lines[min_line].size()) {
            lines[min_line].erase(min_col, max_col - min_col + 1);
            edit_signal.toggle_state();
            return true;
        }
        return false;
    }

    // Handle deletion across multiple lines
    for (int line = min_line; line <= max_line; ++line) {
        if (line == min_line) {
            // Delete from min_col to the end of the line
            if (min_col < lines[line].size()) {
                lines[line].erase(min_col);
            }
        } else if (line == max_line) {
            // Delete from the beginning of the line to max_col
            if (max_col >= 0 && max_col < lines[line].size()) {
                lines[line].erase(0, max_col + 1);
            }
        } else {
            // Delete the entire line
            lines[line].clear();
        }
    }
    edit_signal.toggle_state();
    return true;
}
