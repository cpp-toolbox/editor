#include "text_diff.hpp"

TextModification create_insertion_text_modification(int line, int col, const std::string &text) {
    TextRange range(line, col, line, col);
    return TextModification(range, text, "");
}

TextModification create_newline_diff(int line) {
    // to create a newline we prepend a newline to the current line pushing the next line down
    TextRange range(line, 0, line, 0);
    return TextModification(range, "\n", "");
}

bool is_insertion(const TextModification &modification) {
    bool same_line = modification.text_range_to_replace.start_line == modification.text_range_to_replace.end_line;
    bool same_col = modification.text_range_to_replace.start_col == modification.text_range_to_replace.end_col;
    return same_line and same_col;
}

bool is_newline_insertion(const TextModification &modification) {
    return is_insertion(modification) and modification.new_content == "\n";
}
bool is_newline_deletion(const TextModification &modification) {
    bool lines_differ_by_one =
        modification.text_range_to_replace.start_line + 1 == modification.text_range_to_replace.end_line;
    bool start_of_lines =
        modification.text_range_to_replace.start_col == 0 and modification.text_range_to_replace.end_col == 0;
    return lines_differ_by_one and start_of_lines;
}

/**
 * @brief Creates the inverse of a given TextModification with adjusted range.
 *
 * The inverse modification reverts the changes made by the original modification.
 * The text range is adjusted to match the length of the replacement string.
 *
 * For example, if the original modification replaces a 5-character range with "cat",
 * the inverse modification will target the 3-character range containing "cat".
 *
 *
 * note that we
 *
 * @param modification The original TextModification.
 * @return TextModification The inverse of the original modification.
 */
TextModification get_inverse_modification(const TextModification &modification) {

    if (is_newline_insertion(modification)) {
        // Handle the inverse of a newline insertion
        // We need to remove the inserted newline, so we will delete the range where the newline was inserted.

        // Get the position of the newline insertion
        TextRange adjusted_range(modification.text_range_to_replace.start_line,
                                 modification.text_range_to_replace.start_col,
                                 modification.text_range_to_replace.start_line + 1, // One line after the insertion
                                 modification.text_range_to_replace.start_col // Column where the newline was inserted
        );

        // The original content before the insertion (which might have been replaced with a newline) will be restored
        return TextModification(
            adjusted_range,
            modification.replaced_content, // New content is the inserted newline that we want to remove
            modification.new_content       // Revert to the original content before the newline
        );
    } else {
        // Adjust range for non-newline modification (such as text replacement or deletion)
        TextRange adjusted_range(
            modification.text_range_to_replace.start_line, modification.text_range_to_replace.start_col,
            modification.text_range_to_replace.start_line,
            modification.text_range_to_replace.start_col + static_cast<int>(modification.new_content.size()));

        return TextModification(adjusted_range,
                                modification.replaced_content, // Restore original content
                                modification.new_content       // Original content becomes replaced content
        );
    }
}
