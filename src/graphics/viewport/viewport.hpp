#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "../../utility/text_buffer/text_buffer.hpp"
#include "../../utility/hierarchical_history/hierarchical_history.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

class Viewport {
  public:
    /**
     * @brief Constructs a viewport for a buffer with a specific height.
     * @param buffer The buffer to view.
     * @param cursor_line_offset The initial cursor line offset.
     * @param cursor_col_offset The initial cursor column offset.
     */

    Viewport(std::shared_ptr<LineTextBuffer> initial_buffer, int num_lines, int num_cols, int cursor_line_offset,
             int cursor_col_offset);

    // TODO: this should not exist within the context of a viewport
    // instead it should be with a "ModalEditor" class
    std::vector<std::shared_ptr<LineTextBuffer>> active_file_buffers;

    HierarchicalHistory history;

    std::unordered_map<std::string, std::tuple<int, int>> file_name_to_last_viewport_position;
    void switch_buffers_and_adjust_viewport_position(std::shared_ptr<LineTextBuffer> ltb,
                                                     bool store_movement_to_history = true);

    void scroll(int delta_row, int delta_col);
    void scroll_up();
    void scroll_down();
    void scroll_left();
    void scroll_right();

    char get_symbol_at(int line, int col) const;
    void move_cursor_forward_until_end_of_word();
    void move_cursor_forward_until_next_right_bracket();
    void move_cursor_backward_until_next_left_bracket();
    void move_cursor_forward_by_word();
    void move_cursor_backward_until_start_of_word();
    void move_cursor_backward_by_word();
    void move_cursor_to_start_of_line();
    void move_cursor_to_end_of_line();
    void move_cursor_to_middle_of_line();

    TextModification create_new_line_above_cursor_and_scroll_up();
    const TextModification create_new_line_at_cursor_and_scroll_down();

    TextModification insert_tab_at_cursor();
    TextModification insert_character_at_cursor(char character);
    TextModification insert_string_at_cursor(const std::string &str);
    TextModification insert_character_at(int line, int col, char character);

    TextModification delete_line_at_cursor();
    TextModification delete_character_at_active_position();
    TextModification backspace_at_active_position();

    void set_active_buffer_line_col_under_cursor(int line, int col, bool store_pos_to_history = true);
    void set_active_buffer_line_under_cursor(int line, bool store_pos_to_history = true);
    void set_active_buffer_col_under_cursor(int col, bool store_pos_to_history = true);

    /**
     * @brief Updates the viewport and checks if any cell has changed since the last tick.
     */
    void tick();
    void save_previous_viewport_screen();
    std::vector<std::pair<int, int>> get_changed_cells_since_last_tick() const;
    bool has_cell_changed(int line, int col) const;

    std::shared_ptr<LineTextBuffer> buffer;
    TemporalBinarySignal moved_signal;
    bool selection_mode_on;

    int active_buffer_line_under_cursor;
    int active_buffer_col_under_cursor;
    int num_lines;
    int num_cols;

  private:
    int cursor_line_offset;
    int cursor_col_offset;

    std::vector<std::vector<char>> previous_viewport_screen; ///< Stores the previous state of the viewport.
};

#endif // VIEWPORT_HPP
