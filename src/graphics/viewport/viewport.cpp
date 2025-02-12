#include "viewport.hpp"

#include <cctype> // For std::isalnum

Viewport::Viewport(LineTextBuffer &initial_buffer, int num_lines, int num_cols, int cursor_line_offset,
                   int cursor_col_offset)
    : buffer(initial_buffer), cursor_line_offset(cursor_line_offset), num_lines(num_lines), num_cols(num_cols),
      cursor_col_offset(cursor_col_offset), active_buffer_line_under_cursor(0), active_buffer_col_under_cursor(0),
      selection_mode_on(false) {

    // Initialize the previous_state with the same dimensions as the viewport
    previous_viewport_screen.resize(num_lines, std::vector<char>(num_cols, ' '));
    active_file_buffers.push_back(initial_buffer);
}

void Viewport::switch_buffers_and_adjust_viewport_position(LineTextBuffer &ltb, bool store_movement_to_history) {
    // Save the current active position before switching
    if (!buffer.current_file_path.empty()) {
        file_name_to_last_viewport_position[buffer.current_file_path] =
            std::make_tuple(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    }

    // Switch to the new buffer
    buffer = ltb;

    // Add the buffer to active_file_buffers if it's not already present
    auto it_buffer = std::find_if(active_file_buffers.begin(), active_file_buffers.end(), [&](const LineTextBuffer &b) {
        return b.current_file_path == ltb.current_file_path;
    });
    if (it_buffer == active_file_buffers.end()) {
        active_file_buffers.push_back(ltb);
    }

    // restore the last cursor position if available
    auto it = file_name_to_last_viewport_position.find(ltb.current_file_path);
    if (it != file_name_to_last_viewport_position.end()) {
        auto [line, col] = it->second;
        set_active_buffer_line_col_under_cursor(line, col, store_movement_to_history);
    } else {
        set_active_buffer_line_col_under_cursor(0, 0, store_movement_to_history);
    }
}

void Viewport::tick() {
    // Update the previous state with the current content
    update_previous_state();
}

bool Viewport::has_cell_changed(int line, int col) const {
    // Check if the given cell has changed by comparing current symbol with the previous state
    if (line >= 0 && line < num_lines && col >= 0 && col < num_cols) {
        return get_symbol_at(line, col) != previous_viewport_screen[line][col];
    }
    return false; // If the line/col are out of bounds, return false
}

std::vector<std::pair<int, int>> Viewport::get_changed_cells_since_last_tick() const {
    std::vector<std::pair<int, int>> changed_cells;

    // Iterate over the visible area of the buffer and compare with the previous state
    for (int line = 0; line < num_lines; ++line) {
        for (int col = 0; col < num_cols; ++col) {
            char current_symbol = get_symbol_at(line, col);

            // If the symbol has changed, mark this cell as changed
            if (current_symbol != previous_viewport_screen[line][col]) {
                changed_cells.emplace_back(line, col);
            }
        }
    }

    return changed_cells;
}

void Viewport::update_previous_state() {
    for (int line = 0; line < num_lines; ++line) {
        for (int col = 0; col < num_cols; ++col) {
            previous_viewport_screen[line][col] = get_symbol_at(line, col);
        }
    }
}

void Viewport::scroll(int line_delta, int col_delta) {
    set_active_buffer_line_col_under_cursor(active_buffer_line_under_cursor + line_delta,
                                            active_buffer_col_under_cursor + col_delta);
}

void Viewport::scroll_up() {
    scroll(-1, 0); // Scroll up decreases the row by 1
}

void Viewport::scroll_down() {
    scroll(1, 0); // Scroll down increases the row by 1
}

void Viewport::scroll_left() {
    scroll(0, -1); // Scroll left decreases the column by 1
}

void Viewport::scroll_right() {
    scroll(0, 1); // Scroll right increases the column by 1
}

void Viewport::set_active_buffer_line_col_under_cursor(int line, int col, bool store_pos_to_history) {
    active_buffer_line_under_cursor = line;
    active_buffer_col_under_cursor = col;
    moved_signal.toggle_state();
    if (store_pos_to_history) {
        history.add_flc_to_history(buffer.current_file_path, active_buffer_line_under_cursor,
                                   active_buffer_col_under_cursor);
    }
};

void Viewport::set_active_buffer_line_under_cursor(int line, bool store_pos_to_history) {
    set_active_buffer_line_col_under_cursor(line, active_buffer_col_under_cursor, store_pos_to_history);
}

void Viewport::set_active_buffer_col_under_cursor(int col, bool store_pos_to_history) {
    set_active_buffer_line_col_under_cursor(active_buffer_line_under_cursor, col, store_pos_to_history);
}

char Viewport::get_symbol_at(int line, int col) const {
    int line_index = active_buffer_line_under_cursor + line - cursor_line_offset;
    int column_index = active_buffer_col_under_cursor + col - cursor_col_offset;

    // Check if the line index is within bounds
    if (line_index < buffer.line_count() && line_index >= 0) {
        if (column_index < 0) {
            // Handle negative column indices: Render line number
            std::string line_number = std::to_string(line_index + 1) + "|";
            int line_number_index = column_index + line_number.size();

            if (line_number_index >= 0) {
                return line_number[line_number_index];
            } else {
                return ' '; // Placeholder for out-of-bounds negative positions
            }
        } else {
            // Handle non-negative column indices: Render buffer content
            const std::string &line_content = buffer.get_line(line_index);
            if (column_index < line_content.size()) {
                return line_content[column_index];
            }
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
void Viewport::move_cursor_forward_until_end_of_word() {
    active_buffer_col_under_cursor =
        buffer.find_forward_to_end_of_word(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}
void Viewport::move_cursor_forward_until_next_right_bracket() {
    active_buffer_col_under_cursor =
        buffer.find_column_index_of_next_right_bracket(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}
void Viewport::move_cursor_backward_until_next_left_bracket() {
    active_buffer_col_under_cursor = buffer.find_column_index_of_previous_left_bracket(active_buffer_line_under_cursor,
                                                                                       active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}
void Viewport::move_cursor_forward_by_word() {
    active_buffer_col_under_cursor =
        buffer.find_forward_by_word_index(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}

void Viewport::move_cursor_backward_until_start_of_word() {
    active_buffer_col_under_cursor =
        buffer.find_backward_to_start_of_word(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}

void Viewport::move_cursor_backward_by_word() {
    active_buffer_col_under_cursor =
        buffer.find_backward_by_word_index(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}

bool Viewport::delete_line_at_cursor() {
    // Get the current cursor position
    int line_index = active_buffer_line_under_cursor;

    // Ensure the line index is valid within the buffer's bounds
    if (line_index < 0 || line_index >= buffer.line_count()) {
        return false;
    }

    // Delete the line at the current cursor position
    if (!buffer.delete_line(line_index)) {
        return false;
    }

    // Optionally: Adjust the viewport or cursor position after deletion
    // For example, move the cursor to the previous line after deletion.
    /*if (line_index > 0) {*/
    /*    scroll_up();*/
    /*}*/

    return true;
}

void Viewport::move_cursor_to_start_of_line() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer.line_count()) {
        active_buffer_col_under_cursor = 0; // Move the cursor to the start of the line
    }

    moved_signal.toggle_state();
}

bool Viewport::insert_tab_at_cursor() {
    // Insert a tab at the current cursor position in the buffer
    bool insert_result = buffer.insert_tab(active_buffer_line_under_cursor, active_buffer_col_under_cursor);

    // If insertion is successful, perform scrolling
    if (insert_result) {
        scroll(0, 4); // scroll right by four
    }

    return insert_result; // Return the result of the insert operation
}

void Viewport::move_cursor_to_end_of_line() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer.line_count()) {
        const std::string &line = buffer.get_line(line_index);
        active_buffer_col_under_cursor = line.size(); // Move the cursor to the end of the line
    }

    moved_signal.toggle_state();
}

void Viewport::move_cursor_to_middle_of_line() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer.line_count()) {
        const std::string &line = buffer.get_line(line_index);
        active_buffer_col_under_cursor = line.size() / 2; // Move the cursor to the middle of the line
    }

    moved_signal.toggle_state();
}

bool Viewport::create_new_line_above_cursor_and_scroll_up() {
    // Get the current cursor position
    int line_index = active_buffer_line_under_cursor;

    // Ensure the line index is valid within the buffer's bounds
    if (line_index < 0 || line_index > buffer.line_count()) {
        return false;
    }

    // Insert the new line at the correct position in the buffer
    if (!buffer.insert_blank_line(line_index)) {
        return false;
    }

    // I actualy realized no scrolling is reqiured because it pushes everything down, potential torename func

    set_active_buffer_col_under_cursor(0);
    // Optionally: Adjust any other properties related to the cursor or viewport
    // after inserting the new line, such as moving the cursor to the beginning of the new line.

    return true;
}

bool Viewport::create_new_line_at_cursor_and_scroll_down() {
    // Get the current cursor position
    int line_index = active_buffer_line_under_cursor;

    // Ensure the line index is valid within the buffer's bounds
    if (line_index < 0 || line_index > buffer.line_count()) {
        return false;
    }

    // Insert the new line at the correct position in the buffer
    if (!buffer.insert_blank_line(line_index + 1)) {
        return false;
    }

    scroll_down();
    // and move to the start of the line as well
    set_active_buffer_col_under_cursor(0);
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

bool Viewport::insert_string_at_cursor(const std::string &str) {

    // Attempt to insert the string into the buffer
    if (buffer.insert_string(active_buffer_line_under_cursor, active_buffer_col_under_cursor, str)) {
        // Adjust the cursor column offset to move right after the string insertion
        scroll(0, str.size());
        return true; // Insertion successful
    }

    return false; // Insertion failed (e.g., invalid position)
}
