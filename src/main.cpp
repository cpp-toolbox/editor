// add logic for going up and down in the search menu
// add in multiple textbuffers for a viewport
// eventually get to highlighting, later on though check out
// https://tree-sitter.github.io/tree-sitter/3-syntax-highlighting.html
// check out temp/tree_sitter_...

#include <algorithm>
#include <fmt/core.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <rapidfuzz/fuzz.hpp>

#include "graphics/ui/ui.hpp"
#include "graphics/window/window.hpp"
#include "graphics/shader_cache/shader_cache.hpp"
#include "graphics/batcher/generated/batcher.hpp"
#include "graphics/vertex_geometry/vertex_geometry.hpp"
#include "graphics/texture_atlas/texture_atlas.hpp"
#include "graphics/viewport/viewport.hpp"
#include "graphics/colors/colors.hpp"

#include "utility/fs_utils/fs_utils.hpp"
#include "utility/input_state/input_state.hpp"
#include "utility/glfw_lambda_callback_manager/glfw_lambda_callback_manager.hpp"
#include "utility/limited_vector/limited_vector.hpp"
#include "utility/temporal_binary_signal/temporal_binary_signal.hpp"
#include "utility/text_buffer/text_buffer.hpp"
#include "utility/input_state/input_state.hpp"
#include "utility/regex_command_runner/regex_command_runner.hpp"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <string>
#include <stdexcept>

#include <iostream>
#include <filesystem>
#include <cstdio>

Colors colors;

/**
 * Gets the directory of the executable.
 *
 * @return A string representing the directory where the executable is located.
 */
std::string get_executable_path(char **argv) {
    auto dir = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
    return dir.string(); // Returns the directory as a string
}

std::string extract_filename(const std::string &full_path) {
    std::filesystem::path path(full_path);
    return path.filename().string();
}

std::string get_current_time_string() {
    // Get current time as a time_t object
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // Convert to local time
    std::tm local_tm = *std::localtime(&now_time_t);

    // Format time to string
    std::ostringstream time_stream;
    time_stream << std::put_time(&local_tm, "%Y-%m-%d %H:%M:%S"); // Custom format
    return time_stream.str();
}

bool starts_with_any_prefix(const std::string &str, const std::vector<std::string> &prefixes) {
    for (const auto &prefix : prefixes) {
        if (str.rfind(prefix, 0) == 0) { // Check if 'prefix' is at position 0
            return true;
        }
    }
    return false;
}

// todo fill in the json file for the font we created, then create a shader for absolute position and textures
// then use the texture atlas in conjuction with temp/editor code to render the right characters in the right place
// by re-writing the render function, where instead it will go to the batcher and do a queue draw where we replicate
// the text coords a bunch, no need for the texture packer, but intead just load in the correct image and also
// i thinkt hat we might have correct the stock behavior of the texture atlas which flips the image, use renderdoc for
// that.

enum EditorMode {
    MOVE_AND_EDIT,
    INSERT,
    VISUAL_SELECT,
    COMMAND,
};

class AutomaticKeySequenceCommandRunner {
  public:
    bool command_started = false;

    bool potentially_run_normal_mode_command(std::string partial_command, Viewport &viewport, EditorMode &editor_mode,
                                             TemporalBinarySignal &mode_change_signal, GLFWwindow *window,
                                             bool &fs_browser_is_active) {
        // Command must be more than one character to proceed
        //

        if (partial_command == "rew") {
            fs_browser_is_active = true;
            return true;
        }

        // Handle special commands 'dd' and 'yy'
        if (partial_command == "dd") {
            std::cout << "Special 'dd' command detected: delete current line.\n";
            if (editor_mode == MOVE_AND_EDIT) {
                viewport.delete_line_at_cursor();
            }
            return true;
        }

        if (partial_command == "yy") {
            std::cout << "Special 'yy' command detected: yank current line.\n";
            std::string current_line = viewport.buffer.get_line(viewport.active_buffer_line_under_cursor);
            glfwSetClipboardString(window, current_line.c_str());
            return true;
        }

        // Define operators and regex patterns
        const std::vector<std::string> operator_strings = {"c", "y", "d"};
        const std::vector<std::string> move_toward_match_command_strings = {"f", "F", "t", "T"};
        std::regex move_toward_match_motions(R"(([fFtT])([a-zA-Z0-9]))");
        std::regex word_based_motions(R"(([ebwB]))");

        std::cout << partial_command << std::endl;
        if (starts_with_any_prefix(partial_command, operator_strings) or
            starts_with_any_prefix(partial_command, move_toward_match_command_strings)) {
            std::cout << "command started" << std::endl;
            command_started = true;
        } else {
            std::cout << "command stopped" << std::endl;
            command_started = false;
        }

        if (partial_command.size() <= 1) {
            return false;
        }

        // Extract operator if present
        std::string operator_name;
        if (starts_with_any_prefix(partial_command, operator_strings)) {
            operator_name = partial_command.substr(0, 1); // Extract operator
            partial_command = partial_command.substr(1);  // Remove operator from the command
        }
        // Check for valid motion commands
        bool command_successfully_run = false;
        std::smatch match;
        if (std::regex_match(partial_command, match, move_toward_match_motions)) {
            // Handle move-toward match motions
            std::string motion = match.str(1); // 'f', 'F', 't', or 'T'
            char character = match.str(2)[0];  // The character after the motion

            std::cout << "Move-toward motion command detected:\n";
            std::cout << "  Operator: " << operator_name << "\n";
            std::cout << "  Motion: " << motion << "\n";
            std::cout << "  Character: " << character << "\n";

            int col_idx;
            if (motion == "f") {
                col_idx = viewport.buffer.find_rightward_index(viewport.active_buffer_line_under_cursor,
                                                               viewport.active_buffer_col_under_cursor, character);
            } else if (motion == "F") {
                col_idx = viewport.buffer.find_leftward_index(viewport.active_buffer_line_under_cursor,
                                                              viewport.active_buffer_col_under_cursor, character);
            } else if (motion == "t") {
                col_idx = viewport.buffer.find_rightward_index_before(
                    viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor, character);
            } else if (motion == "T") {
                col_idx = viewport.buffer.find_leftward_index_before(
                    viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor, character);
            }

            bool instance_found = viewport.active_buffer_col_under_cursor != col_idx;

            if (not instance_found) {
                return false;
            }

            if (operator_name.empty()) {
                viewport.set_active_buffer_col_under_cursor(col_idx);
                command_successfully_run = true;
            } else {
                if (operator_name == "d" or operator_name == "c") {
                    viewport.buffer.delete_bounding_box(viewport.active_buffer_line_under_cursor,
                                                        viewport.active_buffer_col_under_cursor,
                                                        viewport.active_buffer_line_under_cursor, col_idx);
                    if (operator_name == "c") {
                        editor_mode = INSERT;
                        mode_change_signal.toggle_state();
                    }

                    command_successfully_run = true;
                }
            }
            return command_successfully_run;
        }

        if (std::regex_match(partial_command, word_based_motions)) {
            // Handle word-based motions
            std::cout << "Word-based motion command detected:\n";
            std::cout << "  Operator: " << operator_name << "\n";
            std::cout << "  Motion: " << partial_command << "\n";

            std::string motion = partial_command;

            int col_idx;
            if (motion == "w") {
                col_idx = viewport.buffer.find_forward_by_word_index(viewport.active_buffer_line_under_cursor,
                                                                     viewport.active_buffer_col_under_cursor);
            } else if (motion == "e") {
                col_idx = viewport.buffer.find_forward_to_end_of_word(viewport.active_buffer_line_under_cursor,
                                                                      viewport.active_buffer_col_under_cursor);
            } else if (motion == "B") {
                col_idx = viewport.buffer.find_backward_by_word_index(viewport.active_buffer_line_under_cursor,
                                                                      viewport.active_buffer_col_under_cursor);
            } else if (motion == "b") {
                col_idx = viewport.buffer.find_backward_to_start_of_word(viewport.active_buffer_line_under_cursor,
                                                                         viewport.active_buffer_col_under_cursor);
            }

            bool instance_found = viewport.active_buffer_col_under_cursor != col_idx;

            if (not instance_found) {
                return false;
            }

            if (operator_name.empty()) {
                viewport.set_active_buffer_col_under_cursor(col_idx);
                command_successfully_run = true;
            } else {
                if (operator_name == "d" or operator_name == "c") {
                    viewport.buffer.delete_bounding_box(viewport.active_buffer_line_under_cursor,
                                                        viewport.active_buffer_col_under_cursor,
                                                        viewport.active_buffer_line_under_cursor, col_idx);
                    if (operator_name == "c") {
                        editor_mode = INSERT;
                        mode_change_signal.toggle_state();
                    }

                    command_successfully_run = true;
                }
            }
            return command_successfully_run;
            /*return run_command(operator_name, partial_command, '\0', viewport, editor_mode, window);*/
        }

        // If no valid command is detected
        std::cout << "Invalid or unrecognized command: " << partial_command << "\n";
        return false;
    }
};

unsigned int SCREEN_WIDTH = 700;
unsigned int SCREEN_HEIGHT = 700;

void adjust_uv_coordinates_in_place(std::vector<glm::vec2> &uv_coords, float horizontal_push, float top_push,
                                    float bottom_push) {
    // Ensure the vector has exactly 4 elements
    if (uv_coords.size() != 4) {
        throw std::invalid_argument("UV coordinates vector must contain exactly 4 elements.");
    }

    // Push in the UV coordinates with separate adjustments for top and bottom
    uv_coords[0].x -= horizontal_push; // Top-right
    uv_coords[0].y += top_push;

    uv_coords[1].x -= horizontal_push; // Bottom-right
    uv_coords[1].y -= bottom_push;

    uv_coords[2].x += horizontal_push; // Bottom-left
    uv_coords[2].y -= bottom_push;

    uv_coords[3].x += horizontal_push; // Top-left
    uv_coords[3].y += top_push;
}

std::string get_mode_string(EditorMode current_mode) {
    switch (current_mode) {
    case MOVE_AND_EDIT:
        return "MOVE_AND_EDIT";
        break;
    case INSERT:
        return "INSERT";
        break;
    case VISUAL_SELECT:
        return "VISUAL_SELECT";
        break;
    case COMMAND:
        return "COMMAND";
        break;
    }
    return "";
}

void render(Viewport &viewport, Grid &screen_grid, Grid &status_bar_grid, Grid &command_bar_grid,
            std::string &command_bar_input, TextureAtlas &monospaced_font_atlas, Batcher &batcher,
            TemporalBinarySignal &mode_change_signal, TemporalBinarySignal &command_bar_input_signal, int center_idx_x,
            int center_idx_y, int num_cols, int num_lines, int col_where_selection_mode_started,
            int line_where_selection_mode_started, EditorMode &current_mode, ShaderCache &shader_cache,
            std::unordered_map<EditorMode, glm::vec4> &mode_to_cursor_color) {

    bool should_replace = viewport.moved_signal.has_just_changed() or viewport.buffer.edit_signal.has_just_changed();

    auto changed_cells = viewport.get_changed_cells_since_last_tick();

    // FILE BUFFER RENDER
    int unique_idx = 0;
    for (int line = 0; line < screen_grid.rows; line++) {
        for (int col = 0; col < screen_grid.cols; col++) {
            auto cell_rect = screen_grid.get_at(col, line);
            IndexedVertices cell_ivs = cell_rect.get_ivs();
            std::string cell_char(1, viewport.get_symbol_at(line, col));
            auto cell_char_tcs = monospaced_font_atlas.get_texture_coordinates_of_sub_texture(cell_char);
            // because the texture has the font inside the cell.
            adjust_uv_coordinates_in_place(cell_char_tcs, 0.017, 0.045, 0.01);

            /*if (viewport.has_cell_changed(line, col)) {*/
            /*    batcher.absolute_position_with_solid_color_shader_batcher.queue_draw(unique_idx,
             * cell_ivs.indices,*/
            /*                                                                         cell_ivs.vertices, true);*/
            /*}*/

            batcher.absolute_position_textured_shader_batcher.queue_draw(
                unique_idx, cell_ivs.indices, cell_ivs.vertices, cell_char_tcs, viewport.has_cell_changed(line, col));
            unique_idx++;
        }
    }

    // STATUS BAR
    std::string mode_string =
        get_mode_string(current_mode) + " | " + extract_filename(viewport.buffer.current_file_path) +
        (viewport.buffer.modified_without_save ? "[+]" : "") + " | " + get_current_time_string() + " |";

    for (int line = 0; line < status_bar_grid.rows; line++) {
        for (int col = 0; col < status_bar_grid.cols; col++) {

            auto cell_rect = status_bar_grid.get_at(col, line);
            IndexedVertices cell_ivs = cell_rect.get_ivs();

            std::string cell_char;
            if (col < mode_string.size()) {
                cell_char = std::string(1, mode_string[col]);
            } else {
                cell_char = "-";
            }

            auto cell_char_tcs = monospaced_font_atlas.get_texture_coordinates_of_sub_texture(cell_char);

            adjust_uv_coordinates_in_place(cell_char_tcs, 0.017, 0.045, 0.01);

            batcher.absolute_position_textured_shader_batcher.queue_draw(
                unique_idx, cell_ivs.indices, cell_ivs.vertices, cell_char_tcs, mode_change_signal.has_just_changed());
            unique_idx++;
        }
    }

    // command bar
    for (int line = 0; line < command_bar_grid.rows; line++) {
        for (int col = 0; col < command_bar_grid.cols; col++) {

            auto cell_rect = command_bar_grid.get_at(col, line);
            IndexedVertices cell_ivs = cell_rect.get_ivs();

            std::string cell_char;
            if (col < command_bar_input.size()) {
                cell_char = std::string(1, command_bar_input[col]);
            } else {
                cell_char = " ";
            }

            auto cell_char_tcs = monospaced_font_atlas.get_texture_coordinates_of_sub_texture(cell_char);

            adjust_uv_coordinates_in_place(cell_char_tcs, 0.017, 0.045, 0.01);

            batcher.absolute_position_textured_shader_batcher.queue_draw(unique_idx, cell_ivs.indices,
                                                                         cell_ivs.vertices, cell_char_tcs,
                                                                         command_bar_input_signal.has_just_changed());
            unique_idx++;
        }
    }

    if (mode_change_signal.has_just_changed()) {
        auto selected_color = mode_to_cursor_color[current_mode];
        shader_cache.set_uniform(ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR, ShaderUniformVariable::RGBA_COLOR,
                                 selected_color);
    }

    if (current_mode == VISUAL_SELECT) {
        int visual_col_delta = -(viewport.active_buffer_col_under_cursor - col_where_selection_mode_started);
        int visual_line_delta = -(viewport.active_buffer_line_under_cursor - line_where_selection_mode_started);

        // Clamp the delta values to ensure they stay within the bounds of the grid
        int clamped_visual_line_delta = std::clamp(center_idx_y + visual_line_delta, 0, num_lines - 1);
        int clamped_visual_col_delta = std::clamp(center_idx_x + visual_col_delta, 0, num_cols - 1);

        // Clamp the starting coordinates to ensure they stay within the bounds of the grid
        int clamped_center_idx_y = std::clamp(center_idx_y, 0, num_lines - 1);
        int clamped_center_idx_x = std::clamp(center_idx_x, 0, num_cols - 1);

        // Call the function with the clamped values
        std::vector<Rectangle> visually_selected_rectangles = screen_grid.get_rectangles_in_bounding_box(
            clamped_visual_line_delta, clamped_visual_col_delta, clamped_center_idx_y, clamped_center_idx_x);

        int obj_id = 1;
        for (auto &rect : visually_selected_rectangles) {
            auto rect_ivs = rect.get_ivs();
            batcher.absolute_position_with_solid_color_shader_batcher.queue_draw(obj_id, rect_ivs.indices,
                                                                                 rect_ivs.vertices, should_replace);
            obj_id++;
        }
    } else { // regular render the cursor in the middle
        auto center_rect = screen_grid.get_at(center_idx_x, center_idx_y);
        auto center_ivs = center_rect.get_ivs();
        batcher.absolute_position_with_solid_color_shader_batcher.queue_draw(0, center_ivs.indices,
                                                                             center_ivs.vertices);
    }

    monospaced_font_atlas.bind_texture();
    batcher.absolute_position_textured_shader_batcher.draw_everything();
    batcher.absolute_position_with_solid_color_shader_batcher.draw_everything();
}

void setup_sdf_shader_uniforms(ShaderCache &shader_cache) {
    auto text_color = glm::vec3(0.5, 0.5, 1);
    float char_width = 0.5;
    float edge_transition = 0.1;

    shader_cache.use_shader_program(ShaderType::TRANSFORM_V_WITH_SIGNED_DISTANCE_FIELD_TEXT);

    shader_cache.set_uniform(ShaderType::TRANSFORM_V_WITH_SIGNED_DISTANCE_FIELD_TEXT, ShaderUniformVariable::TRANSFORM,
                             glm::mat4(1.0f));

    shader_cache.set_uniform(ShaderType::TRANSFORM_V_WITH_SIGNED_DISTANCE_FIELD_TEXT, ShaderUniformVariable::RGB_COLOR,
                             text_color);

    shader_cache.set_uniform(ShaderType::TRANSFORM_V_WITH_SIGNED_DISTANCE_FIELD_TEXT,
                             ShaderUniformVariable::CHARACTER_WIDTH, char_width);

    shader_cache.set_uniform(ShaderType::TRANSFORM_V_WITH_SIGNED_DISTANCE_FIELD_TEXT,
                             ShaderUniformVariable::EDGE_TRANSITION_WIDTH, edge_transition);
    shader_cache.stop_using_shader_program();
}

template <typename Iterable>
std::vector<std::pair<std::string, double>> find_matching_files(const std::string &query, const Iterable &files,
                                                                size_t result_limit) {
    // Initialize results vector
    std::vector<std::pair<std::string, double>> results;

    // Use CachedRatio for efficient repeated comparisons
    rapidfuzz::fuzz::CachedRatio<char> scorer(query);

    // Debugging setup
    spdlog::debug("Starting file matching with query: '{}' and result limit: {}", query, result_limit);

    for (const auto &file : files) {
        // Calculate similarity score
        double score = scorer.similarity(file.string());

        // Log the file and score
        spdlog::debug("File: '{}', Score: {:.2f}", file.string(), score);

        // Add all files with their scores to the results
        results.emplace_back(file.string(), score);
    }

    // Sort results by similarity in descending order
    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

    spdlog::debug("Sorting completed. Total files evaluated: {}", results.size());

    // Trim the results to the specified limit
    if (results.size() > result_limit) {
        results.resize(result_limit);
        spdlog::debug("Trimmed results to the top {} files.", result_limit);
    }

    return results;
}

void update_search_results(std::string &fs_browser_search_query, std::vector<std::filesystem::path> &searchable_files,
                           FileBrowser &fb,
                           std::vector<int> &doids_for_textboxes_for_active_directory_for_later_removal, UI &fs_browser,
                           TemporalBinarySignal &search_results_changed_signal, int &selected_file_doid,
                           std::vector<std::string> &currently_matched_files) {
    // Find matching files
    int file_limit = 10;
    std::vector<std::pair<std::string, double>> matching_files =
        find_matching_files(fs_browser_search_query, searchable_files, file_limit);

    // here we update the list in the UI, but the thing is the ui is screwed up rn so how to do?
    // Display results
    if (matching_files.empty()) {
        std::cout << "No matching files found for query: \"" << fs_browser_search_query << "\"." << std::endl;
    } else {
        Grid file_rows(matching_files.size(), 1, fb.main_file_view_rect);
        auto file_rects = file_rows.get_column(0);

        // clear out old data
        for (auto doid : doids_for_textboxes_for_active_directory_for_later_removal) {
            fs_browser.remove_textbox(doid);
        }
        doids_for_textboxes_for_active_directory_for_later_removal.clear();

        // TODO this is a bad way to do things, use the replace function later on
        fs_browser.remove_textbox(selected_file_doid);
        selected_file_doid = fs_browser.add_textbox(fs_browser_search_query, fb.file_selection_bar, colors.gray40);

        // clear out old data

        // note during this process we delete the old ones and load in the new, so no need to replace.
        // TODO we need to ad da function to remove data from the batcher to make this "complete"
        int i = 0;
        std::cout << "Matching Files (Sorted by Similarity):" << std::endl;

        // clear out old results

        currently_matched_files.clear();
        for (const auto &[file, score] : matching_files) {
            std::cout << file << " (Score: " << score << ")" << std::endl;
            int oid = fs_browser.add_textbox(file, file_rects.at(i), colors.grey);
            doids_for_textboxes_for_active_directory_for_later_removal.push_back(oid);
            currently_matched_files.push_back(file);
            i++;
        }
        search_results_changed_signal.toggle_state();
    }
}

void run_key_logic(InputState &input_state, EditorMode &current_mode, TemporalBinarySignal &mode_change_signal,
                   Viewport &viewport, AutomaticKeySequenceCommandRunner &move_and_edit_arcr,
                   int &line_where_selection_mode_started, int &col_where_selection_mode_started,
                   std::vector<SubTextIndex> &search_results, int &current_search_index, std::string &command_bar_input,
                   TemporalBinarySignal &command_bar_input_signal, GLFWwindow *window, bool &is_search_active,
                   bool &fs_browser_is_active, std::string &fs_browser_search_query,
                   std::vector<std::filesystem::path> &searchable_files, FileBrowser &fb,
                   std::vector<int> &doids_for_textboxes_for_active_directory_for_later_removal, UI &fs_browser,
                   TemporalBinarySignal &search_results_changed_signal, int &selected_file_doid,
                   std::vector<std::string> &currently_matched_files) {

    // making life easier one keystroke at a time.
    std::function<bool(EKey)> jp = [&](EKey k) { return input_state.is_just_pressed(k); };
    std::function<bool(EKey)> ip = [&](EKey k) { return input_state.is_pressed(k); };

    std::vector<std::string> keys_just_pressed_this_tick;
    for (const auto &key : input_state.all_keys) {
        bool char_is_printable =
            key.key_type == KeyType::ALPHA or key.key_type == KeyType::SYMBOL or key.key_type == KeyType::NUMERIC;
        if (char_is_printable and key.pressed_signal.is_just_on()) {
            std::string key_str = key.string_repr;
            if (key.shiftable and input_state.key_enum_to_object.at(EKey::LEFT_SHIFT)->pressed_signal.is_on()) {
                Key shifted_key = *input_state.key_enum_to_object.at(key.key_enum_of_shifted_version);
                key_str = shifted_key.string_repr;
            }
            keys_just_pressed_this_tick.push_back(key_str);
        }
    }

    // if keys just pressed this tick is has length greater or equal to 2, then that implies two keys were pressed ina
    // single tick, should be rare enough to ignore, but note that it may be a cause for later bugs.
    //

    if (fs_browser_is_active) {

        if (not keys_just_pressed_this_tick.empty()) {
            for (const auto &key : keys_just_pressed_this_tick) {
                fs_browser_search_query += key;
            }

            if (searchable_files.empty()) {
                std::cout << "No files found in the search directory." << std::endl;
            } else {
                update_search_results(fs_browser_search_query, searchable_files, fb,
                                      doids_for_textboxes_for_active_directory_for_later_removal, fs_browser,
                                      search_results_changed_signal, selected_file_doid, currently_matched_files);
            }
        }

        if (jp(EKey::ENTER)) {
            if (currently_matched_files.size() != 0) {
                std::cout << "about to load up: " << currently_matched_files[0] << std::endl;
                std::string file_to_open = currently_matched_files[0];
                LineTextBuffer ltb;
                ltb.load_file(file_to_open);
                viewport.buffer = ltb;
                fs_browser_is_active = false;
                return;
            }
        }

        if (jp(EKey::CAPS_LOCK) or jp(EKey::ESCAPE)) {
            std::cout << "tried to turn off fb" << std::endl;
            fs_browser_is_active = false;
            return;
        }
    }

    // if you're running a command
    if (move_and_edit_arcr.command_started) {
        // then forget all other logic and only allow for text entry there
        if (jp(EKey::ESCAPE)) {
            current_mode = MOVE_AND_EDIT;
            mode_change_signal.toggle_state();
        }
        return;
    }

    if (fs_browser_is_active) {
        return;
    }

    if (input_state.is_just_pressed(EKey::ESCAPE)) {
        current_mode = MOVE_AND_EDIT;
        mode_change_signal.toggle_state();
    }

    std::function<void()> shared_m_and_e_and_visual_selection_logic = [&]() {
        if (jp(EKey::j)) {
            viewport.scroll_down();
        }
        if (jp(EKey::k)) {
            viewport.scroll_up();
        }

        if (jp(EKey::h)) {
            viewport.scroll_left();
        }

        if (jp(EKey::l)) {
            viewport.scroll_right();
        }

        if (jp(EKey::w)) {
            viewport.move_cursor_forward_by_word();
        }

        if (jp(EKey::e)) {
            viewport.move_cursor_forward_until_end_of_word();
        }

        if (ip(EKey::LEFT_SHIFT)) {
            viewport.move_cursor_backward_until_start_of_word();
        } else {
            if (jp(EKey::b)) {
                viewport.move_cursor_backward_by_word();
            }
        }
    };

    // we only run our commands if there is not an active command occuring in move and edit mode
    if (not move_and_edit_arcr.command_started) {
        // doing this only cause of the char callback not picking up space
        // potentially just do tihs apprach and get rid of the char callback
        /*if (input_state.is_just_pressed(EKey::SPACE)) {*/
        /*    command_bar_input += " ";*/
        /*    command_bar_input_signal.toggle_state();*/
        /*}*/

        if (current_mode == MOVE_AND_EDIT) {
            shared_m_and_e_and_visual_selection_logic();
            if (ip(EKey::LEFT_CONTROL)) {
                if (jp(EKey::u)) {
                    viewport.scroll(-5, 0);
                }
                if (jp(EKey::d)) {
                    viewport.scroll(5, 0);
                }
            }
            if (jp(EKey::o)) {
                viewport.create_new_line_at_cursor_and_scroll_down();
            }
            if (jp(EKey::v)) {
                current_mode = VISUAL_SELECT;
                mode_change_signal.toggle_state();
                line_where_selection_mode_started = viewport.active_buffer_line_under_cursor;
                col_where_selection_mode_started = viewport.active_buffer_col_under_cursor;
            }
            if (jp(EKey::u)) {
                if (current_mode == MOVE_AND_EDIT) {
                    viewport.buffer.undo();
                }
            }
            if (jp(EKey::r)) {
                if (current_mode == MOVE_AND_EDIT) {
                    viewport.buffer.redo();
                }
            }
            if (ip(EKey::LEFT_SHIFT)) {

                if (jp(EKey::g)) {
                    int last_line_index = viewport.buffer.line_count() - 1;
                    viewport.set_active_buffer_line_under_cursor(last_line_index);
                }

                if (jp(EKey::t)) {
                    viewport.set_active_buffer_line_under_cursor(0);
                }

                if (jp(EKey::m)) {
                    int last_line_index = (viewport.buffer.line_count() - 1) / 2;
                    viewport.set_active_buffer_line_under_cursor(last_line_index);
                }
            }

            if (jp(EKey::LEFT_SHIFT)) {
                if (jp(EKey::n)) {
                    // Check if there are any search results
                    if (!search_results.empty()) {
                        // Move to the previous search result, using forced positive modulo
                        current_search_index =
                            (current_search_index - 1 + search_results.size()) % search_results.size();
                        SubTextIndex sti = search_results[current_search_index];
                        viewport.set_active_buffer_line_col_under_cursor(sti.start_line, sti.start_col);
                    } else {
                        std::cout << "No search results found" << std::endl;
                    }
                }
            } else {
                if (jp(EKey::n)) {
                    std::cout << "next one" << std::endl;

                    if (!search_results.empty()) {
                        current_search_index = (current_search_index + 1) % search_results.size();
                        SubTextIndex sti = search_results[current_search_index];
                        viewport.set_active_buffer_line_col_under_cursor(sti.start_line, sti.start_col);
                    } else {
                        std::cout << "No search results found" << std::endl;
                    }
                }
            }
            if (input_state.is_pressed(EKey::LEFT_SHIFT)) {
                if (jp(EKey::SEMICOLON)) {
                    current_mode = COMMAND;
                    command_bar_input = ":";
                    command_bar_input_signal.toggle_state();
                    mode_change_signal.toggle_state();
                }
            }
            if (jp(EKey::SLASH)) {
                current_mode = COMMAND;
                command_bar_input = "/";
                command_bar_input_signal.toggle_state();
                mode_change_signal.toggle_state();
            }

        } else if (current_mode == COMMAND) {
            // only doing this cause space isn't handled by the char callback
            if (jp(EKey::ENTER)) {
                if (command_bar_input == ":w") {
                    viewport.buffer.save_file();
                }
                if (command_bar_input == ":q") {
                    glfwSetWindowShouldClose(window, true);
                }
                if (command_bar_input.front() == '/') {
                    // Forward search command
                    std::string search_request = command_bar_input.substr(1); // remove the "/"
                    search_results =
                        viewport.buffer.find_forward_matches(viewport.active_buffer_line_under_cursor,
                                                             viewport.active_buffer_col_under_cursor, search_request);
                    if (!search_results.empty()) {
                        std::cout << "search active true now" << std::endl;
                        current_search_index = 0; // start from the first result

                        // Print out matches
                        std::cout << "Search Results for '" << search_request << "':\n";
                        for (const auto &result : search_results) {
                            // Assuming SubTextIndex has `line` and `col` attributes for position
                            std::cout << "Match at Line: " << result.start_line << ", Column: " << result.start_col
                                      << "\n";
                            // If you want to print the actual text matched:
                            /*std::cout << "Matched text: " << matched_text << "\n";*/
                        }
                        // You may want to highlight the first search result here
                        // highlight_search_result(search_results[current_search_index]);
                    }
                }

                command_bar_input = "";
                command_bar_input_signal.toggle_state();
                current_mode = MOVE_AND_EDIT;
                mode_change_signal.toggle_state();
            }
        }
    }

    if (current_mode == INSERT) {
        if (jp(EKey::ENTER)) {
            viewport.create_new_line_at_cursor_and_scroll_down();
        }
    } else if (current_mode == VISUAL_SELECT) {
        shared_m_and_e_and_visual_selection_logic();
    }
}

int main(int argc, char *argv[]) {

    RegexCommandRunner rcr;
    std::cout << get_executable_path(argv) << std::endl;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("mwe_shader_cache_logs.txt", true);
    file_sink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};

    bool start_in_fullscreen = false;
    GLFWwindow *window = initialize_glfw_glad_and_return_window(SCREEN_WIDTH, SCREEN_HEIGHT, "glfw window",
                                                                start_in_fullscreen, false, false);

    std::vector<int> doids_for_textboxes_for_active_directory_for_later_removal;
    TemporalBinarySignal search_results_changed_signal;

    std::vector<ShaderType> requested_shaders = {
        ShaderType::ABSOLUTE_POSITION_TEXTURED,
        ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR,
        ShaderType::TRANSFORM_V_WITH_SIGNED_DISTANCE_FIELD_TEXT,
        ShaderType::ABSOLUTE_POSITION_WITH_COLORED_VERTEX,
    };

    ShaderCache shader_cache(requested_shaders, sinks);
    Batcher batcher(shader_cache);

    setup_sdf_shader_uniforms(shader_cache);

    std::filesystem::path font_info_path =
        std::filesystem::path("assets") / "fonts" / "times_64_sdf_atlas_font_info.json";
    std::filesystem::path font_json_path = std::filesystem::path("assets") / "fonts" / "times_64_sdf_atlas.json";
    std::filesystem::path font_image_path = std::filesystem::path("assets") / "fonts" / "times_64_sdf_atlas.png";
    FontAtlas font_atlas(font_info_path.string(), font_json_path.string(), font_image_path.string(), SCREEN_WIDTH,
                         false, true);

    bool fs_browser_is_active = false;
    std::string fs_browser_search_query = "";
    std::string search_dir = ".";
    std::vector<std::string> ignore_dirs = {"build", ".git", "__pycache__"};
    std::vector<std::filesystem::path> searchable_files = rec_get_all_files(search_dir, ignore_dirs);
    for (const auto &file : searchable_files) {
        std::cout << file.string() << '\n';
    }
    UI fs_browser(font_atlas);
    FileBrowser fb(1.5, 1.5);

    std::string temp = "File Search";
    std::string select = "select a file";
    std::vector<std::string> currently_matched_files;
    fs_browser.add_colored_rectangle(fb.background_rect, colors.gray10);
    int curr_dir_doid = fs_browser.add_textbox(temp, fb.current_directory_rect, colors.gold);
    fs_browser.add_colored_rectangle(fb.main_file_view_rect, colors.gray40);
    int selected_file_doid = fs_browser.add_textbox(select, fb.file_selection_bar, colors.gray40);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    InputState input_state;
    std::cout << "after constructor" << std::endl;

    std::unordered_map<EditorMode, glm::vec4> mode_to_cursor_color = {
        {MOVE_AND_EDIT, {.5, .5, .5, .5}},
        {INSERT, {.8, .8, .5, .5}},
        {VISUAL_SELECT, {.8, .5, .8, .5}},
        {COMMAND, {.8, .5, .5, .5}},
    };

    shader_cache.set_uniform(ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR, ShaderUniformVariable::RGBA_COLOR,
                             mode_to_cursor_color[MOVE_AND_EDIT]);

    int width, height;

    TextureAtlas monospaced_font_atlas("assets/font/font.json", "assets/font/font.png");

    // numbers must be odd to have a center
    int num_lines = 41;
    int num_cols = 101;

    int line_where_selection_mode_started = -1;
    int col_where_selection_mode_started = -1;

    float status_bar_top_pos = -0.90;
    float command_bar_top_pos = -0.95;
    float top_line_pos = 1;

    Rectangle file_buffer_rect =
        create_rectangle_from_corners(glm::vec3(-1, top_line_pos, 0), glm::vec3(1, top_line_pos, 0),
                                      glm::vec3(-1, status_bar_top_pos, 0), glm::vec3(1, status_bar_top_pos, 0));

    Rectangle status_bar_rect =
        create_rectangle_from_corners(glm::vec3(-1, status_bar_top_pos, 0), glm::vec3(1, status_bar_top_pos, 0),
                                      glm::vec3(-1, command_bar_top_pos, 0), glm::vec3(1, command_bar_top_pos, 0));

    Rectangle command_bar_rect =
        create_rectangle_from_corners(glm::vec3(-1, command_bar_top_pos, 0), glm::vec3(1, command_bar_top_pos, 0),
                                      glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0));

    /*Grid file_buffer_grid(num_lines, num_cols, */
    Grid screen_grid(num_lines, num_cols, file_buffer_rect);
    Grid status_bar_grid(1, num_cols, status_bar_rect);
    Grid command_bar_grid(1, num_cols, command_bar_rect);

    AutomaticKeySequenceCommandRunner move_and_edit_arcr;
    std::vector<SubTextIndex> search_results;
    int current_search_index = 0;  // to keep track of the current search result
    bool is_search_active = false; // to check if a search is in progress

    std::string potential_automatic_command;
    std::string command_bar_input;
    TemporalBinarySignal command_bar_input_signal;

    int center_idx_x = num_cols / 2;
    int center_idx_y = num_lines / 2;

    glm::vec2 active_visible_screen_position;

    // Check if the user provided a file argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1; // Return an error code if no argument is provided
    }

    std::string filename = argv[1]; // Get the file name from the first argument

    std::vector<LineTextBuffer> active_file_buffers;
    LineTextBuffer file_buffer;
    active_file_buffers.push_back(file_buffer);

    LineTextBuffer file_info;
    LineTextBuffer command_line;
    if (!file_buffer.load_file(filename)) {
        return 1; // Return an error code if the file couldn't be loaded
    }

    EditorMode current_mode = MOVE_AND_EDIT;
    TemporalBinarySignal mode_change_signal;
    Viewport viewport(file_buffer, num_lines, num_cols, center_idx_y, center_idx_x);

    TemporalBinarySignal insert_mode_signal;

    std::function<void(unsigned int)> char_callback = [&](unsigned int character_code) {
        if (fs_browser_is_active) {
        } else {
            if (current_mode == INSERT) {

                // update the thing or else it gets stuck
                if (insert_mode_signal.next_has_just_changed()) {
                    return;
                }

                // Convert the character code to a character
                char character = static_cast<char>(character_code);

                // Insert the character at the current cursor position
                if (!viewport.insert_character_at_cursor(character)) {
                    // Handle the case where the insertion failed
                    std::cerr << "Failed to insert character at cursor position.\n";
                }
            }

            if (current_mode == COMMAND) {
                // Convert the character code to a character
                char character = static_cast<char>(character_code);
                command_bar_input += character;
                command_bar_input_signal.toggle_state();
            }
        }
    };

    // Define the key callback
    std::function<void(int, int, int, int)> key_callback = [&](int key, int scancode, int action, int mods) {
        // these events happen once when the key is pressed down, aka its non-repeating; a one time event

        if (action == GLFW_PRESS || action == GLFW_RELEASE) {

            Key &active_key = *input_state.glfw_code_to_key.at(key);
            bool is_pressed = (action == GLFW_PRESS);
            active_key.pressed_signal.set_signal(is_pressed);

            Key &enum_grabbed_key = *input_state.key_enum_to_object.at(active_key.key_enum);

            if (current_mode == MOVE_AND_EDIT && command_bar_input == ":") {
                command_bar_input = "";
                command_bar_input_signal.toggle_state();
            }

            if (current_mode == MOVE_AND_EDIT) {
                if (action == GLFW_PRESS) {
                    if (active_key.key_type == KeyType::ALPHA or active_key.key_type == KeyType::NUMERIC or
                        active_key.string_repr == "escape") {

                        if (mods & GLFW_MOD_SHIFT) {
                            if (active_key.shiftable) {
                                active_key = *input_state.key_enum_to_object.at(active_key.key_enum_of_shifted_version);
                            }
                        }
                        std::string key_str = active_key.string_repr;

                        // print out the key that was just pressed
                        std::cout << "key_str:" << key_str << std::endl;

                        if (key_str == "u" && viewport.buffer.get_last_deleted_content() == "") {
                            command_bar_input = "Ain't no more history!";
                            command_bar_input_signal.toggle_state();
                        }

                        // maybe this can be generalized to unary, binary etc type commands
                        std::vector<std::string> potential_automatic_command_prefixes = {"d", "c", "y", "f", "F", "t",
                                                                                         "T",
                                                                                         // temp adding theses
                                                                                         "r", "e", "w"};

                        bool command_started_or_continuing =
                            starts_with_any_prefix(key_str, potential_automatic_command_prefixes) or
                            move_and_edit_arcr.command_started;
                        bool control_not_pressed =
                            input_state.key_enum_to_object.at(EKey::LEFT_CONTROL)->pressed_signal.is_off();
                        if (key_str != "escape") {
                            if (command_started_or_continuing and control_not_pressed) {
                                potential_automatic_command += key_str;
                                std::cout << "command: " << potential_automatic_command << std::endl;
                                std::cout << "----------" << std::endl;

                                command_bar_input = potential_automatic_command;
                                command_bar_input_signal.toggle_state();

                                if (potential_automatic_command.length() > 3) {
                                    potential_automatic_command = potential_automatic_command.substr(1);
                                }
                            }
                        }
                        bool command_was_run = move_and_edit_arcr.potentially_run_normal_mode_command(
                            potential_automatic_command, viewport, current_mode, mode_change_signal, window,
                            fs_browser_is_active);
                        if (command_was_run | key_str == "escape") {
                            potential_automatic_command = "";
                            command_bar_input = "";
                            command_bar_input_signal.toggle_state();
                            std::cout << "command ended: " << potential_automatic_command << std::endl;
                            std::cout << "----------" << std::endl;
                        }
                    }
                    if (active_key.key_enum == EKey::ESCAPE or active_key.key_enum == EKey::CAPS_LOCK) {
                        potential_automatic_command = "";
                    }
                }
            }
        }

        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key) {
                // this case switch is so BS, need to use the frag-z thing for handling keyboard input, take a look at
                // that again.
            case GLFW_KEY_I:
                if (current_mode == MOVE_AND_EDIT) {
                    current_mode = INSERT;
                    insert_mode_signal.toggle_state();
                    mode_change_signal.toggle_state();
                }
                break;
            case GLFW_KEY_CAPS_LOCK:
                current_mode = MOVE_AND_EDIT;
                mode_change_signal.toggle_state();
                break;
            case GLFW_KEY_X:
                if (current_mode == MOVE_AND_EDIT) {
                    viewport.delete_character_at_active_position();
                }

                if (current_mode == VISUAL_SELECT) {
                    viewport.buffer.delete_bounding_box(
                        line_where_selection_mode_started, col_where_selection_mode_started,
                        viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor);

                    viewport.set_active_buffer_line_col_under_cursor(line_where_selection_mode_started,
                                                                     col_where_selection_mode_started);

                    current_mode = MOVE_AND_EDIT;
                    mode_change_signal.toggle_state();
                }
                break;
            case GLFW_KEY_BACKSPACE:
                if (fs_browser_is_active) {
                    std::cout << "search backspace" << std::endl;
                    fs_browser_search_query =
                        fs_browser_search_query.empty()
                            ? ""
                            : fs_browser_search_query.substr(0, fs_browser_search_query.size() - 1);
                    update_search_results(fs_browser_search_query, searchable_files, fb,
                                          doids_for_textboxes_for_active_directory_for_later_removal, fs_browser,
                                          search_results_changed_signal, selected_file_doid, currently_matched_files);
                } else {
                    std::cout << "non seach backspace" << std::endl;
                    if (current_mode == INSERT) {
                        viewport.backspace_at_active_position();
                    }
                }
                break;
            case GLFW_KEY_4:                 // $ key (Shift + 4)
                if (mods & GLFW_MOD_SHIFT) { // Check if Shift is pressed
                    if (current_mode == MOVE_AND_EDIT || current_mode == VISUAL_SELECT) {
                        viewport.move_cursor_to_end_of_line();
                    }
                }
                break;
            case GLFW_KEY_0:
                if (current_mode == MOVE_AND_EDIT || current_mode == VISUAL_SELECT) {
                    viewport.move_cursor_to_start_of_line();
                }
                break;
            case GLFW_KEY_M: // m key (no modifier)
                if (current_mode == MOVE_AND_EDIT || current_mode == VISUAL_SELECT) {
                    viewport.move_cursor_to_middle_of_line();
                }
                break;
            case GLFW_KEY_TAB:
                if (current_mode == INSERT) {
                    viewport.insert_tab_at_cursor();
                }
                break;
            case GLFW_KEY_P:
                if (current_mode == MOVE_AND_EDIT) {
                    if (mods & GLFW_MOD_SHIFT) {
                        // Shift is held down, insert content from the clipboard
                        const char *clipboard_content = glfwGetClipboardString(window);
                        if (clipboard_content) {
                            viewport.insert_string_at_cursor(clipboard_content);
                        }
                    } else {
                        // Insert the last deleted content
                        viewport.insert_string_at_cursor(viewport.buffer.get_last_deleted_content());
                    }
                }
                break;
            case GLFW_KEY_Y:
                if (current_mode == VISUAL_SELECT) {
                    std::string curr_sel = viewport.buffer.get_bounding_box_string(
                        line_where_selection_mode_started, col_where_selection_mode_started,
                        viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor);
                    glfwSetClipboardString(window, curr_sel.c_str());
                }
                break;
            default:
                break;
            }
        }
    };
    std::function<void(double, double)> mouse_pos_callback = [](double _, double _1) {};
    std::function<void(int, int, int)> mouse_button_callback = [](int _, int _1, int _2) {};
    GLFWLambdaCallbackManager glcm(window, char_callback, key_callback, mouse_pos_callback, mouse_button_callback);

    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render(viewport, screen_grid, status_bar_grid, command_bar_grid, command_bar_input, monospaced_font_atlas,
               batcher, mode_change_signal, command_bar_input_signal, center_idx_x, center_idx_y, num_cols, num_lines,
               col_where_selection_mode_started, line_where_selection_mode_started, current_mode, shader_cache,
               mode_to_cursor_color);

        // render UI stuff

        if (fs_browser_is_active) {

            for (auto &cb : fs_browser.get_colored_boxes()) {
                batcher.absolute_position_with_colored_vertex_shader_batcher.queue_draw(
                    cb.id, cb.ivpsc.indices, cb.ivpsc.xyz_positions, cb.ivpsc.rgb_colors);
            }

            for (auto &tb : fs_browser.get_text_boxes()) {
                bool should_change = search_results_changed_signal.has_just_changed();
                batcher.absolute_position_with_colored_vertex_shader_batcher.queue_draw(
                    tb.id, tb.background_ivpsc.indices, tb.background_ivpsc.xyz_positions,
                    tb.background_ivpsc.rgb_colors);

                batcher.transform_v_with_signed_distance_field_text_shader_batcher.queue_draw(
                    tb.id, tb.text_drawing_data.indices, tb.text_drawing_data.xyz_positions,
                    tb.text_drawing_data.texture_coordinates, tb.modified_signal.has_just_changed());
            }

            for (auto &tb : fs_browser.get_clickable_text_boxes()) {
                batcher.absolute_position_with_colored_vertex_shader_batcher.queue_draw(
                    tb.id, tb.ivpsc.indices, tb.ivpsc.xyz_positions, tb.ivpsc.rgb_colors,
                    tb.modified_signal.has_just_changed());

                batcher.transform_v_with_signed_distance_field_text_shader_batcher.queue_draw(
                    tb.id, tb.text_drawing_data.indices, tb.text_drawing_data.xyz_positions,
                    tb.text_drawing_data.texture_coordinates);
            }

            /*glDisable(GL_DEPTH_TEST);*/
            font_atlas.texture_atlas.bind_texture();
            batcher.absolute_position_with_colored_vertex_shader_batcher.draw_everything();
            batcher.transform_v_with_signed_distance_field_text_shader_batcher.draw_everything();
            /*glEnable(GL_DEPTH_TEST);*/
        }

        // render UI stuff

        // not sure why this has to go here right now but it doesn't update if it comes after mofifying viewport.
        viewport.tick();

        run_key_logic(input_state, current_mode, mode_change_signal, viewport, move_and_edit_arcr,
                      line_where_selection_mode_started, col_where_selection_mode_started, search_results,
                      current_search_index, command_bar_input, command_bar_input_signal, window, is_search_active,
                      fs_browser_is_active, fs_browser_search_query, searchable_files, fb,
                      doids_for_textboxes_for_active_directory_for_later_removal, fs_browser,
                      search_results_changed_signal, selected_file_doid, currently_matched_files);

        TemporalBinarySignal::process_all();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
