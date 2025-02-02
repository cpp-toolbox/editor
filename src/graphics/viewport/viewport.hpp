#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "../../utility/text_buffer/text_buffer.hpp"
#include <vector>

class Viewport {
  public:
    /**
     * @brief Constructs a viewport for a buffer with a specific height.
     * @param buffer The buffer to view.
     * @param cursor_line_offset The initial cursor line offset.
     * @param cursor_col_offset The initial cursor column offset.
     */

    Viewport(LineTextBuffer &buffer, int num_lines, int num_cols, int cursor_line_offset, int cursor_col_offset);

    void scroll(int delta_row, int delta_col);
    void scroll_up();
    void scroll_down();
    void scroll_left();
    void scroll_right();

    char get_symbol_at(int line, int col) const;
    void move_cursor_to(int line, int col);
    void move_cursor_forward_until_end_of_word(); 
    void move_cursor_forward_until_next_right_bracket();
    void move_cursor_backward_until_next_left_bracket();
    void move_cursor_forward_by_word();
    void move_cursor_backward_until_start_of_word();
    void move_cursor_backward_by_word();
    void move_cursor_to_start_of_line();
    void move_cursor_to_end_of_line();
    void move_cursor_to_middle_of_line();

    bool create_new_line_at_cursor_and_scroll_down();
    bool insert_tab_at_cursor();
    bool insert_character_at_cursor(char character);
    bool insert_string_at_cursor(const std::string &str);
    bool insert_character_at(int line, int col, char character);

    bool delete_line_at_cursor();
    bool delete_character_at_active_position();
    bool backspace_at_active_position();

    void set_active_buffer_line_col_under_cursor(int line, int col);
    void set_active_buffer_line_under_cursor(int line);
    void set_active_buffer_col_under_cursor(int col);

    /**
     * @brief Updates the viewport and checks if any cell has changed since the last tick.
     */
    void tick();
    std::vector<std::pair<int, int>> get_changed_cells_since_last_tick() const;
    bool has_cell_changed(int line, int col) const;

    LineTextBuffer &buffer; ///< Reference to the buffer being viewed.
    TemporalBinarySignal moved_signal;
    bool selection_mode_on;

    int active_buffer_line_under_cursor;
    int active_buffer_col_under_cursor;
    int num_lines;
    int num_cols;

  private:
    int cursor_line_offset;
    int cursor_col_offset;

    std::vector<std::vector<char>> previous_state; ///< Stores the previous state of the viewport.

    void update_previous_state();
};

#endif // VIEWPORT_HPP
