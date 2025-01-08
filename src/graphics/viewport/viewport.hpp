#ifndef VIEWPORT_HPP
#define VIEWPORT_HPP

#include "../../utility/text_buffer/text_buffer.hpp"

class Viewport {
  public:
    /**
     * @brief Constructs a viewport for a buffer with a specific height.
     * @param buffer The buffer to view.
     * @param cursor_line_offset The initial cursor line offset.
     * @param cursor_col_offset The initial cursor column offset.
     */
    Viewport(LineTextBuffer &buffer, int cursor_line_offset, int cursor_col_offset);

    /**
     * @brief Scrolls the viewport up by one line.
     */
    void scroll_up();

    /**
     * @brief Scrolls the viewport down by one line.
     */
    void scroll_down();

    /**
     * @brief Scrolls the viewport left by one column.
     */
    void scroll_left();

    /**
     * @brief Scrolls the viewport right by one column.
     */
    void scroll_right();

    /**
     * @brief Gets the character at a specified line and column in the viewport.
     * @param line The line index in the viewport.
     * @param col The column index in the viewport.
     * @return The character at the specified position, or ' ' if out of bounds.
     */
    char get_symbol_at(int line, int col) const;

    /**
     * @brief Deletes a character at the current viewport position and scrolls left.
     * @param line The line index in the viewport.
     * @param col The column index in the viewport.
     * @return True if the character was successfully deleted, false otherwise.
     */
    bool delete_character_at_active_position();

    bool backspace_at_active_position();

    /**
     * @brief Inserts a character at the current viewport position and scrolls right.
     * @param line The line index in the viewport.
     * @param col The column index in the viewport.
     * @param character The character to insert.
     * @return True if the character was successfully inserted, false otherwise.
     */
    bool insert_character_at(int line, int col, char character);

    /**
     * @brief Moves the cursor to the next word boundary.
     * This function jumps until it hits a non-word character, similar to 'w' in vim.
     */
    void move_cursor_forward_by_word();

    /**
     * @brief Moves the cursor to the previous word boundary.
     * This function jumps backward until it hits a non-word character, similar to 'b' in vim.
     */
    void move_cursor_backward_by_word();

    bool create_new_line_at_cursor_and_scroll_down();

    bool insert_character_at_cursor(char character);

    LineTextBuffer &buffer; ///< Reference to the buffer being viewed.
    TemporalBinarySignal moved_signal;
    bool selection_mode_on;

    int active_buffer_line_under_cursor;
    int active_buffer_col_under_cursor;

  private:
    int cursor_line_offset;
    int cursor_col_offset;
};

#endif // VIEWPORT_HPP
