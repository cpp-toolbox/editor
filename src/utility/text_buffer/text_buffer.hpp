#ifndef TEXT_BUFFER_HPP
#define TEXT_BUFFER_HPP

#include <string>
#include <vector>
#include <stack>

#include "../temporal_binary_signal/temporal_binary_signal.hpp"
#include "../text_diff/text_diff.hpp"

class LineTextBuffer {
  private:
    std::vector<std::string> lines;
    std::stack<TextModification> undo_stack;
    std::stack<TextModification> redo_stack;

  public:
    TemporalBinarySignal edit_signal;
    std::string current_file_path;
    bool modified_without_save = false;

    static constexpr const std::string TAB = "    ";

    bool load_file(const std::string &file_path);
    bool save_file();

    std::string get_text() const;

    int line_count() const;
    std::string get_line(int line_index) const;
    std::string get_bounding_box_string(int start_line, int start_col, int end_line, int end_col) const;
    std::string get_text_from_range(const TextRange &range) const;
    bool character_is_non_word_character(const std::string &c) const;

    // NOTE: MODIFICATION FUNCTIONS START

    TextModification apply_text_modification(const TextModification &modification);
    TextModification delete_character(int line_index, int col_index);
    std::vector<TextModification> delete_bounding_box(int start_line, int start_col, int end_line, int end_col);
    TextModification delete_line(int line_index);

    TextModification replace_line(int line_index, const std::string &new_content);
    TextModification append_line(const std::string &line);

    TextModification insert_character(int line_index, int col_index, char character);
    TextModification insert_string(int line_index, int col_index, const std::string &str);
    TextModification insert_newline_after_this_line(int line_index);
    TextModification insert_tab(int line_index, int col_index);

    // NOTE: MODIFICATION FUNCTIONS END

    int find_rightward_index(int line_index, int col_index, char character) const;
    int find_leftward_index(int line_index, int col_index, char character) const;
    int find_rightward_index_before(int line_index, int col_index, char character) const;
    int find_leftward_index_before(int line_index, int col_index, char character) const;

    int find_col_idx_of_first_non_whitespace_character_in_line(int line_index) const;

    int find_forward_by_word_index(int line_index, int col_index) const;
    int find_forward_to_end_of_word(int line_index, int col_index) const;

    int find_column_index_of_next_character(int line_index, int col_index, char target_char) const;
    int find_column_index_of_character_leftward(int line_index, int col_index, char target_char) const;

    int find_column_index_of_next_right_bracket(int line_index, int col_index) const;
    int find_column_index_of_previous_left_bracket(int linx_index, int col_index) const;

    int find_backward_by_word_index(int line_index, int col_index) const;
    int find_backward_to_start_of_word(int line_index, int col_index) const;

    std::vector<TextRange> find_forward_matches(int line_index, int col_index, const std::string &regex_str) const;
    std::vector<TextRange> find_backward_matches(int line_index, int col_index, const std::string &regex_str) const;

    std::string get_last_deleted_content() const;

    int get_indentation_level(int line, int col) const;

    TextModification undo();
    TextModification redo();
};

#endif // TEXT_BUFFER_HPP
