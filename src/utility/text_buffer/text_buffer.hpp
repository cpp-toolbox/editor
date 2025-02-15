#ifndef TEXT_BUFFER_HPP
#define TEXT_BUFFER_HPP

#include <string>
#include <vector>
#include <stack>

#include "../temporal_binary_signal/temporal_binary_signal.hpp"
#include "../text_diff/text_diff.hpp"

// TODO: legacy diff which is being depricated
class Diff {
  public:
    enum class Type { INSERT, REPLACE, DELETE_LINE, DELETE_WITHIN_LINE };

    Type type;
    int line_index;
    int col_index;
    std::string text;

    Diff(Type t, int line, int col, const std::string &text_data)
        : type(t), line_index(line), col_index(col), text(text_data) {}
};

class LineTextBuffer {
  private:
    std::vector<std::string> lines;
    std::stack<Diff> undo_stack;
    std::stack<Diff> redo_stack;

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

    TextDiff delete_character(int line_index, int col_index);
    bool delete_bounding_box(int start_line, int start_col, int end_line, int end_col);
    bool delete_line(int line_index);

    bool replace_line(int line_index, const std::string &new_content);
    void append_line(const std::string &line);

    TextDiff insert_character(int line_index, int col_index, char character);
    bool insert_string(int line_index, int col_index, const std::string &str);
    TextDiff insert_new_line(int line_index);
    TextDiff insert_tab(int line_index, int col_index);

    int find_rightward_index(int line_index, int col_index, char character);
    int find_leftward_index(int line_index, int col_index, char character);
    int find_rightward_index_before(int line_index, int col_index, char character);
    int find_leftward_index_before(int line_index, int col_index, char character);

    int find_col_idx_of_first_non_whitespace_character_in_line(int line_index);

    int find_forward_by_word_index(int line_index, int col_index);
    int find_forward_to_end_of_word(int line_index, int col_index);

    int find_column_index_of_next_character(int line_index, int col_index, char target_char);
    int find_column_index_of_character_leftward(int line_index, int col_index, char target_char);

    int find_column_index_of_next_right_bracket(int line_index, int col_index);
    int find_column_index_of_previous_left_bracket(int linx_index, int col_index);

    int find_backward_by_word_index(int line_index, int col_index);
    int find_backward_to_start_of_word(int line_index, int col_index);

    std::vector<TextRange> find_forward_matches(int line_index, int col_index, const std::string &regex_str);
    std::vector<TextRange> find_backward_matches(int line_index, int col_index, const std::string &regex_str);

    std::string get_last_deleted_content() const;

    int get_indentation_level(int line, int col);

    void undo();
    void redo();
};

#endif // TEXT_BUFFER_HPP
