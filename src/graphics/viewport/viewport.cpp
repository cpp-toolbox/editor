#include "viewport.hpp"

#include <cctype> // For std::isalnum

Viewport::Viewport(std::shared_ptr<LineTextBuffer> initial_buffer, int num_lines, int num_cols, int cursor_line_offset,
                   int cursor_col_offset)
    : buffer(initial_buffer), cursor_line_offset(cursor_line_offset), num_lines(num_lines), num_cols(num_cols),
      cursor_col_offset(cursor_col_offset), active_buffer_line_under_cursor(0), active_buffer_col_under_cursor(0),
      selection_mode_on(false) {

    // Initialize the previous_state with the same dimensions as the viewport
    previous_viewport_screen.resize(num_lines, std::vector<char>(num_cols, ' '));
    active_file_buffers.push_back(initial_buffer);
}

void Viewport::switch_buffers_and_adjust_viewport_position(std::shared_ptr<LineTextBuffer> ltb,
                                                           bool store_movement_to_history) {
    // Save the current active position before switching
    if (!buffer->current_file_path.empty()) {
        file_name_to_last_viewport_position[buffer->current_file_path] =
            std::make_tuple(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    }

    buffer = ltb;

    auto it_buffer = std::find_if(
        active_file_buffers.begin(), active_file_buffers.end(),
        [&](const std::shared_ptr<LineTextBuffer> &b) { return b->current_file_path == ltb->current_file_path; });

    if (it_buffer == active_file_buffers.end()) {
        active_file_buffers.push_back(ltb);
    }

    // restore the last cursor position if available
    auto it = file_name_to_last_viewport_position.find(ltb->current_file_path);
    if (it != file_name_to_last_viewport_position.end()) {
        auto [line, col] = it->second;
        set_active_buffer_line_col_under_cursor(line, col, store_movement_to_history);
    } else {
        set_active_buffer_line_col_under_cursor(0, 0, store_movement_to_history);
    }
}

void Viewport::tick() {
    // Update the previous state with the current content
    save_previous_viewport_screen();
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

void Viewport::save_previous_viewport_screen() {
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
        history.add_flc_to_history(buffer->current_file_path, active_buffer_line_under_cursor,
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
    if (line_index < buffer->line_count() && line_index >= 0) {
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
            const std::string &line_content = buffer->get_line(line_index);
            if (column_index < line_content.size()) {
                return line_content[column_index];
            }
        }
    }
    return ' '; // Placeholder for out-of-bounds positions
}

TextModification Viewport::delete_character_at_active_position() {
    return buffer->delete_character(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
}

TextModification Viewport::backspace_at_active_position() {
    auto td = buffer->delete_character(active_buffer_line_under_cursor, active_buffer_col_under_cursor - 1);
    if (td != EMPTY_TEXT_DIFF) {
        scroll_left();
    }
    return td;
}

TextModification Viewport::insert_character_at(int line, int col, char character) {
    int line_index = active_buffer_line_under_cursor + line;
    int column_index = active_buffer_col_under_cursor + col;

    auto td = buffer->insert_character(line_index, column_index, character);

    if (td != EMPTY_TEXT_DIFF) {
        scroll_right();
    }
    return td;
}
void Viewport::move_cursor_forward_until_end_of_word() {
    active_buffer_col_under_cursor =
        buffer->find_forward_to_end_of_word(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}
void Viewport::move_cursor_forward_until_next_right_bracket() {
    active_buffer_col_under_cursor = buffer->find_column_index_of_next_right_bracket(active_buffer_line_under_cursor,
                                                                                     active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}
void Viewport::move_cursor_backward_until_next_left_bracket() {
    active_buffer_col_under_cursor = buffer->find_column_index_of_previous_left_bracket(active_buffer_line_under_cursor,
                                                                                        active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}
void Viewport::move_cursor_forward_by_word() {
    active_buffer_col_under_cursor =
        buffer->find_forward_by_word_index(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}

void Viewport::move_cursor_backward_until_start_of_word() {
    active_buffer_col_under_cursor =
        buffer->find_backward_to_start_of_word(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}

void Viewport::move_cursor_backward_by_word() {
    active_buffer_col_under_cursor =
        buffer->find_backward_by_word_index(active_buffer_line_under_cursor, active_buffer_col_under_cursor);
    moved_signal.toggle_state();
}

TextModification Viewport::delete_line_at_cursor() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < 0 || line_index >= buffer->line_count()) {
        return EMPTY_TEXT_DIFF;
    }

    auto tm = buffer->delete_line(line_index);
    return tm;
}

void Viewport::move_cursor_to_start_of_line() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer->line_count()) {
        active_buffer_col_under_cursor = 0; // Move the cursor to the start of the line
    }

    moved_signal.toggle_state();
}

TextModification Viewport::insert_tab_at_cursor() {
    // Insert a tab at the current cursor position in the buffer
    auto td = buffer->insert_tab(active_buffer_line_under_cursor, active_buffer_col_under_cursor);

    if (td != EMPTY_TEXT_DIFF) {
        scroll(0, buffer->TAB.size());
    }

    return td;
}

void Viewport::move_cursor_to_end_of_line() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer->line_count()) {
        const std::string &line = buffer->get_line(line_index);
        active_buffer_col_under_cursor = line.size(); // Move the cursor to the end of the line
    }

    moved_signal.toggle_state();
}

void Viewport::move_cursor_to_middle_of_line() {
    int line_index = active_buffer_line_under_cursor;

    if (line_index < buffer->line_count()) {
        const std::string &line = buffer->get_line(line_index);
        active_buffer_col_under_cursor = line.size() / 2; // Move the cursor to the middle of the line
    }

    moved_signal.toggle_state();
}

TextModification Viewport::create_new_line_above_cursor_and_scroll_up() {
    auto td = buffer->insert_newline_after_this_line(active_buffer_line_under_cursor);

    // TODO: I actualy realized no scrolling is reqiured because it pushes everything down, potentially rename func
    set_active_buffer_col_under_cursor(0);

    return td;
}

const TextModification Viewport::create_new_line_at_cursor_and_scroll_down() {
    int line_index = active_buffer_line_under_cursor;
    auto const td = buffer->insert_newline_after_this_line(line_index);

    if (td != EMPTY_TEXT_DIFF) {
        scroll_down();
        // and move to the start of the line as well
        set_active_buffer_col_under_cursor(0);
        // Optionally: Adjust any other properties related to the cursor or viewport
        // after inserting the new line, such as moving the cursor to the beginning of the new line.
    }

    return td;
}

TextModification Viewport::insert_character_at_cursor(char character) {
    auto tm = buffer->insert_character(active_buffer_line_under_cursor, active_buffer_col_under_cursor, character);
    scroll_right();
    return tm;
}

TextModification Viewport::insert_string_at_cursor(const std::string &str) {
    auto tm = buffer->insert_string(active_buffer_line_under_cursor, active_buffer_col_under_cursor, str);

    if (tm != EMPTY_TEXT_DIFF) {
        scroll(0, str.size());
    }

    return tm;
}
