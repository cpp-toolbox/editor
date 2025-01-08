#ifndef TEXT_BUFFER_HPP
#define TEXT_BUFFER_HPP

#include <string>
#include <vector>
#include "../temporal_binary_signal/temporal_binary_signal.hpp"

class LineTextBuffer {
  public:
    /**
     * @brief Loads a file into the buffer.
     * @param file_path The path to the file to load.
     * @return True if the file was successfully loaded, false otherwise.
     */
    bool load_file(const std::string &file_path);

    /**
     * @brief Saves the current buffer to the file it was loaded from.
     * @return True if the file was successfully saved, false otherwise.
     */
    bool save_file() const;

    /**
     * @brief Gets the number of lines in the buffer.
     * @return The number of lines in the buffer.
     */
    int line_count() const;

    /**
     * @brief Gets a line from the buffer at the specified index.
     * @param line_index The index of the line to retrieve.
     * @return The line at the specified index, or an empty string if out of bounds.
     */
    std::string get_line(int line_index) const;

    /**
     * @brief Deletes a character from a specific position in a line.
     * @param line_index The index of the line to edit.
     * @param col_index The index of the character to delete.
     * @return True if the character was successfully deleted, false otherwise.
     */
    bool delete_character(int line_index, int col_index);

    /**
     * @brief Inserts a character at a specific position in a line.
     * @param line_index The index of the line to edit.
     * @param col_index The index at which to insert the character.
     * @param character The character to insert.
     * @return True if the character was successfully inserted, false otherwise.
     */
    bool insert_character(int line_index, int col_index, char character);

    /**
     * @brief Deletes an entire line from the buffer.
     * @param line_index The index of the line to delete.
     * @return True if the line was successfully deleted, false otherwise.
     */
    bool delete_line(int line_index);

    /**
     * @brief Appends a new line to the buffer.
     * @param line The line to append.
     */
    void append_line(const std::string &line);

    /**
     * @brief Replaces the content of a line at a specified index.
     * @param line_index The index of the line to replace.
     * @param new_content The new content for the line.
     * @return True if the line was successfully replaced, false otherwise.
     */
    bool replace_line(int line_index, const std::string &new_content);

    /**
     * @brief Inserts a blank line at a specific index in the buffer.
     * @param line_index The index at which to insert the blank line.
     * @return True if the blank line was successfully inserted, false otherwise.
     */
    bool insert_blank_line(int line_index);

    /**
     * @brief Deletes all characters within the bounding box defined by two (line, col) pairs.
     * @param start_line The starting line of the bounding box.
     * @param start_col The starting column of the bounding box.
     * @param end_line The ending line of the bounding box.
     * @param end_col The ending column of the bounding box.
     * @return True if the characters were successfully deleted, false otherwise.
     */
    bool delete_bounding_box(int start_line, int start_col, int end_line, int end_col);

    TemporalBinarySignal edit_signal;

  private:
    std::vector<std::string> lines; ///< The lines of text stored in the buffer.
    std::string current_file_path;  ///< Path to the file currently loaded in the buffer.
};

#endif // TEXT_BUFFER_HPP
