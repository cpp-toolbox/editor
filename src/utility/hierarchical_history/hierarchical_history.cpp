#include "hierarchical_history.hpp"

HierarchicalHistory::HierarchicalHistory() : current_index(-1) {}

void HierarchicalHistory::add_flc_to_history(const std::string& filepath, int line, int col) {
	// remove the active history position, push it to the end of history
	// NOTE: doesn't do anything when you're at the end of history already
	// and that is expected behavior as it will not change the history
    if (current_index >= 0 && current_index < file_line_col_history.size()) {
        auto curr_flc = file_line_col_history[current_index];
        file_line_col_history.erase(file_line_col_history.begin() + current_index);
        file_line_col_history.push_back(curr_flc);
    }

    file_line_col_history.push_back(std::make_tuple(filepath, line, col));

    // move the current pointer to the end
    current_index = file_line_col_history.size() - 1;
}

void HierarchicalHistory::go_back() {
    if (current_index > 0) {
        current_index--;
    }
}

void HierarchicalHistory::go_forward() {
    if (current_index < file_line_col_history.size() - 1) {
        current_index++;
    }
}

std::tuple<std::string, int, int> HierarchicalHistory::get_current_history_flc() const {
    return (current_index >= 0 && current_index < file_line_col_history.size()) ? file_line_col_history[current_index] : std::make_tuple("", -1, -1);
}

void HierarchicalHistory::display_history() const {
    std::cout << "History: [";
    for (size_t i = 0; i < file_line_col_history.size(); ++i) {
        const auto& [filepath, line, col] = file_line_col_history[i];
        std::cout << "(" << filepath << ", " << line << ", " << col << ")";
        if (i != file_line_col_history.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << "]" << std::endl;
}

void HierarchicalHistory::display_pointer() const {
    std::cout << "Current Pointer: " << current_index << std::endl;
}
