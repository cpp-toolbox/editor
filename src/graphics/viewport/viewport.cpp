#include "viewport.hpp"

#include <cctype> // For std::isalnum

Viewport::Viewport(LineTextBuffer &buffer, int cursor_line_offset, int cursor_col_offset)
    : buffer(buffer), active_buffer_line_under_cursor(0), active_buffer_col_under_cursor(0),
      cursor_line_offset(cursor_line_offset), cursor_col_offset(cursor_col_offset) {}

void Viewport::scroll_up() {
    --active_buffer_line_under_cursor;
    moved_signal.toggle_state();
}

void Viewport::scroll_down() {
    ++active_buffer_line_under_cursor;
    moved_signal.toggle_state();
}

void Viewport::scroll_left() {
    --active_buffer_col_under_cursor;
    moved_signal.toggle_state();
}

void Viewport::scroll_right() {
    ++active_buffer_col_under_cursor;
    moved_signal.toggle_state();
}

char Viewport::get_symbol_at(int line, int col) const {
    int line_index = active_buffer_line_under_cursor + line - cursor_line_offset;
    int column_index = active_buffer_col_under_cursor + col - cursor_col_offset;

    if (line_index < buffer.line_count()) {
        const std::string &line = buffer.get_line(line_index);
        if (column_index < line.size()) {
            return line[column_index];
        }
    }
    return ' '; // Placeholder for out-of-bounds positions
}

bool Viewport::delete_character_at_active_position() {
    if (buffer.delete_character(active_buffer_line_under_cursor, active_buffer_col_under_cursor)) {
        return true;
    }
    return false;
}

bool Viewport::backspace_at_active_position() {
    if (buffer.delete_character(active_buffer_line_under_cursor, active_buffer_col_under_cursor - 1)) {
        scroll_left();
        return true;
    }
    return false;
}

bool Viewport::insert_character_at(int line, int col, char character) {
    int line_index = active_buffer_line_under_cursor + line;
    int column_index = active_buffer_col_under_cursor + col;

    if (buffer.insert_character(line_index, column_index, character)) {
        scroll_right();
        return true;
    }
    return false;
}

void Viewport::move_cursor_forward_by_word() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer.line_count()) {
        const std::string &line = buffer.get_line(line_index);

        // Skip alphanumeric characters
        while (active_buffer_col_under_cursor < line.size() && std::isalnum(line[active_buffer_col_under_cursor])) {
            ++active_buffer_col_under_cursor;
        }

        // Skip non-alphanumeric characters
        while (active_buffer_col_under_cursor < line.size() && !std::isalnum(line[active_buffer_col_under_cursor])) {
            ++active_buffer_col_under_cursor;
        }
    }

    moved_signal.toggle_state();
}

void Viewport::move_cursor_backward_by_word() {
    int line_index = active_buffer_line_under_cursor;
    if (line_index < buffer.line_count()) {
        const std::string &line = buffer.get_line(line_index);

        // Skip non-alphanumeric characters backward
        while (active_buffer_col_under_cursor > 0 && !std::isalnum(line[active_buffer_col_under_cursor - 1])) {
            --active_buffer_col_under_cursor;
        }

        // Skip alphanumeric characters backward
        while (active_buffer_col_under_cursor > 0 && std::isalnum(line[active_buffer_col_under_cursor - 1])) {
            --active_buffer_col_under_cursor;
        }
    }

    moved_signal.toggle_state();
}

bool Viewport::create_new_line_at_cursor_and_scroll_down() {
    // Get the current cursor position
    int line_index = active_buffer_line_under_cursor;

    // Ensure the line index is valid within the buffer's bounds
    if (line_index < 0 || line_index > buffer.line_count()) {
        return false;
    }

    // Create a new blank line at the specified position
    std::string new_line = "";

    // Insert the new line at the correct position in the buffer
    if (!buffer.insert_blank_line(line_index + 1)) {
        return false;
    }

    scroll_down();

    // Optionally: Adjust any other properties related to the cursor or viewport
    // after inserting the new line, such as moving the cursor to the beginning of the new line.

    return true;
}

bool Viewport::insert_character_at_cursor(char character) {

    // Attempt to insert the character into the buffer
    if (buffer.insert_character(active_buffer_line_under_cursor, active_buffer_col_under_cursor, character)) {
        // Adjust the cursor column offset to move right after insertion
        scroll_right();
        return true; // Insertion successful
    }

    return false; // Insertion failed (e.g., invalid position)
}
