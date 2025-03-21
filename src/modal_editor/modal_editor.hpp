#ifndef MODAL_EDITOR_HPP
#define MODAL_EDITOR_HPP

#include "../utility/hierarchical_history/hierarchical_history.hpp"
#include "../utility/temporal_binary_signal/temporal_binary_signal.hpp"
#include "../utility/text_diff/text_diff.hpp"
#include "../utility/regex_command_runner/regex_command_runner.hpp"

enum EditorMode {
    MOVE_AND_EDIT,
    INSERT,
    VISUAL_SELECT,
    COMMAND,
};

class ModalEditor {
  public:
    // main stuff start
    EditorMode current_mode = MOVE_AND_EDIT;
    TemporalBinarySignal mode_change_signal;

    // main stuff end

    // rcr start
    RegexCommandRunner rcr;
    bool configured_rcr = false;
    std::string potential_regex_command = "";
    // rcr end

    // searching within buffer start
    std::vector<TextRange> search_results;
    int current_search_index = 0; // to keep track of the current search result
    bool is_search_active = false;
    // searching within buffer end

    // fsb start
    bool fs_browser_is_active = false;
    std::string fs_browser_search_query = "";
    TemporalBinarySignal search_results_changed_signal;
    std::vector<std::string> currently_matched_files;
    // fsb end

    // afb start
    bool afb_is_active = false;
    std::string afb_search_query = "";
    TemporalBinarySignal afb_search_results_changed_signal;
    std::vector<std::string> afb_currently_matched_files;
    // afb end
};

#endif // MODAL_EDITOR_HPP
