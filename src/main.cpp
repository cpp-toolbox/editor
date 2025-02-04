// add logic for going up and down in the search menu
// add in multiple textbuffers for a viewport
// eventually get to highlighting, later on though check out
// https://tree-sitter.github.io/tree-sitter/3-syntax-highlighting.html
// check out temp/tree_sitter_...

#include <algorithm>
#include <fmt/core.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <nlohmann/detail/input/input_adapters.hpp>
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
#include "utility/periodic_signal/periodic_signal.hpp"
#include "utility/config_file_parser/config_file_parser.hpp"

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
            std::unordered_map<EditorMode, glm::vec4> &mode_to_cursor_color, double delta_time,
            PeriodicSignal &one_second_signal_for_status_bar_time_update) {

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

    bool should_update_status_bar = one_second_signal_for_status_bar_time_update.process_and_get_signal();

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
                unique_idx, cell_ivs.indices, cell_ivs.vertices, cell_char_tcs, should_update_status_bar);
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
                                                                size_t result_limit, double filename_weight = 0.7) {
    std::vector<std::pair<std::string, double>> results;

    rapidfuzz::fuzz::CachedRatio<char> scorer(query);
    spdlog::debug("Starting file matching with query: '{}' and result limit: {}", query, result_limit);

    for (const auto &file : files) {
        std::string file_path = file.string();
        std::string filename = std::filesystem::path(file_path).filename().string();

        // Calculate similarity scores for both the full path and the filename
        double path_score = scorer.similarity(file_path);
        double filename_score = scorer.similarity(filename);

        // Weighted combination of both scores
        double combined_score = (1.0 - filename_weight) * path_score + filename_weight * filename_score;

        spdlog::debug("File: '{}', Path Score: {:.2f}, Filename Score: {:.2f}, Combined Score: {:.2f}", file_path,
                      path_score, filename_score, combined_score);

        results.emplace_back(file_path, combined_score);
    }

    std::sort(results.begin(), results.end(), [](const auto &a, const auto &b) { return a.second > b.second; });

    spdlog::debug("Sorting completed. Total files evaluated: {}", results.size());

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
                   Viewport &viewport, int &line_where_selection_mode_started, int &col_where_selection_mode_started,
                   std::vector<SubTextIndex> &search_results, int &current_search_index, std::string &command_bar_input,
                   TemporalBinarySignal &command_bar_input_signal, Window window, bool &is_search_active,
                   bool &fs_browser_is_active, std::string &fs_browser_search_query,
                   std::vector<std::filesystem::path> &searchable_files, FileBrowser &fb,
                   std::vector<int> &doids_for_textboxes_for_active_directory_for_later_removal, UI &fs_browser,
                   TemporalBinarySignal &search_results_changed_signal, int &selected_file_doid,
                   std::vector<std::string> &currently_matched_files, RegexCommandRunner &rcr,
                   std::string &potential_regex_command, bool &configured_rcr,
                   TemporalBinarySignal &insert_mode_signal) {

    if (not configured_rcr) {
        rcr.add_regex("^x", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.delete_character_at_active_position();
            }

            if (current_mode == VISUAL_SELECT) {
                viewport.buffer.delete_bounding_box(line_where_selection_mode_started, col_where_selection_mode_started,
                                                    viewport.active_buffer_line_under_cursor,
                                                    viewport.active_buffer_col_under_cursor);

                viewport.set_active_buffer_line_col_under_cursor(line_where_selection_mode_started,
                                                                 col_where_selection_mode_started);

                current_mode = MOVE_AND_EDIT;
                mode_change_signal.toggle_state();
            }
        });
        rcr.add_regex("^m", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT || current_mode == VISUAL_SELECT) {
                viewport.move_cursor_to_middle_of_line();
            }
        });
        rcr.add_regex("^\\$", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT || current_mode == VISUAL_SELECT) {
                viewport.move_cursor_to_end_of_line();
            }
        });
        rcr.add_regex("^[pP]", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                if (m.str(0) == "P") {
                    // Uppercase 'P' - insert content from the clipboard
                    const char *clipboard_content = glfwGetClipboardString(window.glfw_window);
                    if (clipboard_content) {
                        viewport.insert_string_at_cursor(clipboard_content);
                    }
                } else if (m.str(0) == "p") {
                    // Lowercase 'p' - insert the last deleted content
                    viewport.insert_string_at_cursor(viewport.buffer.get_last_deleted_content());
                }
            }
        });
        rcr.add_regex("^0", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT || current_mode == VISUAL_SELECT) {
                viewport.move_cursor_to_start_of_line();
            }
        });

        rcr.add_regex("v", [&](const std::smatch &m) {
            current_mode = VISUAL_SELECT;
            mode_change_signal.toggle_state();
            line_where_selection_mode_started = viewport.active_buffer_line_under_cursor;
            col_where_selection_mode_started = viewport.active_buffer_col_under_cursor;
        });

        rcr.add_regex("^i", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                current_mode = INSERT;
                insert_mode_signal.toggle_state();
                mode_change_signal.toggle_state();
            }
        });
        rcr.add_regex("^a", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.scroll_right();
                current_mode = INSERT;
                insert_mode_signal.toggle_state();
                mode_change_signal.toggle_state();
            }
        });
        rcr.add_regex("^A", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.move_cursor_to_end_of_line();
                current_mode = INSERT;
                insert_mode_signal.toggle_state();
                mode_change_signal.toggle_state();
            }
        });
        rcr.add_regex("^I", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                int ciofnwc = viewport.buffer.find_col_idx_of_first_non_whitespace_character_in_line(
                    viewport.active_buffer_line_under_cursor);
                viewport.set_active_buffer_col_under_cursor(ciofnwc);
                current_mode = INSERT;
                insert_mode_signal.toggle_state();
                mode_change_signal.toggle_state();
            }
        });
        rcr.add_regex("^\\^", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                int ciofnwc = viewport.buffer.find_col_idx_of_first_non_whitespace_character_in_line(
                    viewport.active_buffer_line_under_cursor);
                viewport.set_active_buffer_col_under_cursor(ciofnwc);
            }
        });
        rcr.add_regex(R"(^(\d*)([jklh]))", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT or current_mode == VISUAL_SELECT) {
                int count = m[1].str().empty() ? 1 : std::stoi(m[1].str());
                char direction = m[2].str()[0];

                int line_delta = 0, col_delta = 0;
                switch (direction) {
                case 'j':
                    if (viewport.buffer.line_count() > viewport.active_buffer_line_under_cursor + 1) {
                        line_delta = count;
                    }
                    break; // Scroll down
                case 'k':
                    if (viewport.active_buffer_line_under_cursor > 0) {
                        line_delta = -count;
                    }
                    break; // Scroll up
                case 'h':
                    if (viewport.active_buffer_col_under_cursor > 0) {
                        col_delta = -count;
                    }
                    break; // Scroll left
                case 'l':
                    const std::string &line = viewport.buffer.get_line(viewport.active_buffer_line_under_cursor);

                    // TODO: this is bad because it stops me from being able to scroll rightware
                    // if I want to on a blank line which is a real use case because sometimes
                    // I want to be able to scroll right without caring about where I am in the file
                    // note that this behavior is different than vim and thats ok
                    /*if (line.size() > viewport.active_buffer_col_under_cursor) {*/
                    /*    col_delta = count;*/
                    /*}*/

                    col_delta = count;

                    break; // Scroll right
                }

                std::cout << "line delta: " << line_delta << ", col delta: " << col_delta << std::endl;
                viewport.scroll(line_delta, col_delta);
            }
        });
        rcr.add_regex(R"((\d*)G)", [&](const std::smatch &m) {
            std::string digits = m[1].str();

            if (digits.empty()) {
                int last_line_index = viewport.buffer.line_count() - 1;
                viewport.set_active_buffer_line_under_cursor(last_line_index);
            } else {
                int number = std::stoi(digits);
                viewport.set_active_buffer_line_under_cursor(number - 1);
            }
        });
        rcr.add_regex(R"(([cd]?)([fFtT])(.))", [&](const std::smatch &m) {
            std::string action = m[1].str(); // 'c' (change) or 'd' (delete), optional
            std::string motion = m[2].str(); // 'f', 'F', 't', or 'T'
            char character = m[3].str()[0];  // The character to search for

            int col_idx = -1;

            // Handle motion commands
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

            // Apply action based on motion result
            if (col_idx != -1) {
                if (!action.empty()) {
                    // If action is 'c' or 'd', handle deletion and mode change
                    viewport.buffer.delete_bounding_box(viewport.active_buffer_line_under_cursor,
                                                        viewport.active_buffer_col_under_cursor,
                                                        viewport.active_buffer_line_under_cursor, col_idx);
                    if (action == "c") {
                        current_mode = INSERT;
                        mode_change_signal.toggle_state();
                    }
                } else {
                    viewport.set_active_buffer_col_under_cursor(col_idx);
                }
            } else {
                std::cout << "Character '" << character << "' not found for motion '" << motion << "'.\n";
            }
        });
        // Regex for [cd][webB] (change/delete with word motions)
        rcr.add_regex(R"(^([cd]?)([webB]))", [&](const std::smatch &m) {
            std::string command = m[1].str();
            std::string motion = m[2].str();

            // If no command is provided (meaning [cd] is missing), run these default motions
            if (command.empty()) {
                if (motion == "w") {
                    viewport.move_cursor_forward_by_word();
                } else if (motion == "e") {
                    viewport.move_cursor_forward_until_end_of_word();
                } else if (motion == "b") {
                    viewport.move_cursor_backward_by_word();
                } else if (motion == "B") {
                    viewport.move_cursor_backward_until_start_of_word();
                }
                return;
            }

            // otherwise run the deletion stuff
            int col_idx = -1;
            if (motion == "w") {
                // minus one is used here to mimic default vim behavior and not to delet the first character of the next
                // word
                col_idx = viewport.buffer.find_forward_by_word_index(viewport.active_buffer_line_under_cursor,
                                                                     viewport.active_buffer_col_under_cursor) -
                          1;
            } else if (motion == "e") {
                col_idx = viewport.buffer.find_forward_to_end_of_word(viewport.active_buffer_line_under_cursor,
                                                                      viewport.active_buffer_col_under_cursor);
            } else if (motion == "b") {
                col_idx = viewport.buffer.find_backward_to_start_of_word(viewport.active_buffer_line_under_cursor,
                                                                         viewport.active_buffer_col_under_cursor);
            } else if (motion == "B") {
                col_idx = viewport.buffer.find_backward_by_word_index(viewport.active_buffer_line_under_cursor,
                                                                      viewport.active_buffer_col_under_cursor);
            }

            // Apply action based on motion result
            if (col_idx != -1) {
                viewport.buffer.delete_bounding_box(viewport.active_buffer_line_under_cursor,
                                                    viewport.active_buffer_col_under_cursor,
                                                    viewport.active_buffer_line_under_cursor, col_idx);
                if (command == "c") {
                    current_mode = INSERT;
                    insert_mode_signal.toggle_state();
                    mode_change_signal.toggle_state();
                }
            }
        });
        // modification within brackets
        rcr.add_regex(R"(([cd])([ai])([bB]))", [&](const std::smatch &m) {
            std::string command = m[1].str();
            std::string inside_or_around = m[2].str();
            std::string bracket_type = m[3].str();

            int left_match_col = -1;
            int right_match_col = -1;

            if (bracket_type == "b") {
                left_match_col = viewport.buffer.find_column_index_of_previous_left_bracket(
                    viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor);
                right_match_col = viewport.buffer.find_column_index_of_next_right_bracket(
                    viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor);
            }

            if (bracket_type == "B") {
                left_match_col = viewport.buffer.find_column_index_of_character_leftward(
                    viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor, '{');
                right_match_col = viewport.buffer.find_column_index_of_next_character(
                    viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor, '}');
            }

            if (left_match_col == -1 or right_match_col == -1) {
                return;
            }

            if (inside_or_around == "i") {
                left_match_col++;
                right_match_col--;
            }

            viewport.buffer.delete_bounding_box(viewport.active_buffer_line_under_cursor, right_match_col,
                                                viewport.active_buffer_line_under_cursor, left_match_col);
            viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, left_match_col);
        });

        rcr.add_regex("^gg", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.set_active_buffer_line_under_cursor(0);
            }
        });
        rcr.add_regex("^[oO]", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                if (m.str() == "O") {
                    // Create a new line above the cursor and scroll up
                    viewport.create_new_line_above_cursor_and_scroll_up();
                } else {
                    // Create a new line below the cursor and scroll down
                    viewport.create_new_line_at_cursor_and_scroll_down();
                }

                current_mode = INSERT;
                insert_mode_signal.toggle_state();
                mode_change_signal.toggle_state();
            }
        });

        rcr.add_regex("^u", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.buffer.undo();
            }
        });
        rcr.add_regex("^r", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.buffer.redo();
            }
        });
        rcr.add_regex("^ sf", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                fs_browser_is_active = true;
            }
        });
        rcr.add_regex("^dd", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                viewport.delete_line_at_cursor();
            }
        });
        rcr.add_regex("^yy", [&](const std::smatch &m) {
            if (current_mode == MOVE_AND_EDIT) {
                std::string current_line = viewport.buffer.get_line(viewport.active_buffer_line_under_cursor);
                glfwSetClipboardString(window.glfw_window, current_line.c_str());
            }
        });

        configured_rcr = true;
    }

    // making life easier one keystroke at a time.
    std::function<bool(EKey)> jp = [&](EKey k) { return input_state.is_just_pressed(k); };
    std::function<bool(EKey)> ip = [&](EKey k) { return input_state.is_pressed(k); };
    auto keys_just_pressed_this_tick = input_state.get_keys_just_pressed_this_tick();
    if (not fs_browser_is_active) {

        // NOTE: if keys just pressed this tick is has length greater or equal to 2, then that implies two keys were
        // pressed ina single tick, should be rare enough to ignore, but note that it may be a cause for later bugs.
        if (not keys_just_pressed_this_tick.empty()) {
            for (const auto &key : keys_just_pressed_this_tick) {
                potential_regex_command += key;
            }
            std::cout << "prc: " << potential_regex_command << std::endl;
        }

        bool command_was_run = rcr.potentially_run_command(potential_regex_command);
        if (command_was_run) {
            potential_regex_command = "";
        }

        if (input_state.is_just_pressed(EKey::ESCAPE) or input_state.is_just_pressed(EKey::CAPS_LOCK)) {
            potential_regex_command = "";
        }

        bool key_pressed_based_command_run = false;
        if (current_mode == MOVE_AND_EDIT) {
            if (ip(EKey::LEFT_CONTROL)) {
                if (jp(EKey::u)) {
                    viewport.scroll(-5, 0);
                    key_pressed_based_command_run = true;
                }
                if (jp(EKey::d)) {
                    viewport.scroll(5, 0);
                    key_pressed_based_command_run = true;
                }
            }
            if (ip(EKey::LEFT_SHIFT)) {

                if (jp(EKey::m)) {
                    int last_line_index = (viewport.buffer.line_count() - 1) / 2;
                    viewport.set_active_buffer_line_under_cursor(last_line_index);
                    key_pressed_based_command_run = true;
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
                    key_pressed_based_command_run = true;
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

                    key_pressed_based_command_run = true;
                }
            }
            if (input_state.is_pressed(EKey::LEFT_SHIFT)) {
                if (jp(EKey::SEMICOLON)) {
                    current_mode = COMMAND;
                    command_bar_input = ":";
                    command_bar_input_signal.toggle_state();
                    mode_change_signal.toggle_state();

                    key_pressed_based_command_run = true;
                }
            }
            if (jp(EKey::SLASH)) {
                current_mode = COMMAND;
                command_bar_input = "/";
                command_bar_input_signal.toggle_state();
                mode_change_signal.toggle_state();
                key_pressed_based_command_run = true;
            }

        } else if (current_mode == COMMAND) {
            // only doing this cause space isn't handled by the char callback
            if (jp(EKey::ENTER)) {
                if (command_bar_input == ":w") {
                    viewport.buffer.save_file();
                    key_pressed_based_command_run = true;
                }
                if (command_bar_input == ":q") {
                    glfwSetWindowShouldClose(window.glfw_window, true);
                    key_pressed_based_command_run = true;
                }
                if (command_bar_input == ":tfs") {
                    window.toggle_fullscreen();
                    key_pressed_based_command_run = true;
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

        if (key_pressed_based_command_run) {
            potential_regex_command = "";
        }

    } else {

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
                std::cout << "file_to_open: " << file_to_open << std::endl;

                bool found_active_file_buffer = false;
                for (auto &active_file_buffer : viewport.active_file_buffers) {
                    std::cout << active_file_buffer.current_file_path << std::endl;
                    if (active_file_buffer.current_file_path == file_to_open) {
                        std::cout << "found matching buffer, using it " << std::endl;
                        viewport.switch_buffers(active_file_buffer);
                        found_active_file_buffer = true;
                        fs_browser_is_active = false;
                        fs_browser_search_query = "";
                    }
                }
                if (not found_active_file_buffer) {
                    std::cout << "didn't find matching buffer creating new buffer" << std::endl;
                    LineTextBuffer ltb;
                    ltb.load_file(file_to_open);
                    viewport.switch_buffers(ltb);
                    fs_browser_is_active = false;
                    fs_browser_search_query = "";
                }
                return;
            }
        }

        if (jp(EKey::CAPS_LOCK) or jp(EKey::ESCAPE)) {
            std::cout << "tried to turn off fb" << std::endl;
            fs_browser_is_active = false;
            return;
        }
    }

    if (fs_browser_is_active) {
        return;
    }

    if (input_state.is_just_pressed(EKey::ESCAPE)) {
        current_mode = MOVE_AND_EDIT;
        mode_change_signal.toggle_state();
    }

    std::function<void()> shared_m_and_e_and_visual_selection_logic = [&]() {};

    if (current_mode == INSERT) {
        if (jp(EKey::ENTER)) {
            viewport.create_new_line_at_cursor_and_scroll_down();
        }
    }
}

// code to make sure the cursor stays within the lines
void snap_to_end_of_line_while_navigating(Viewport &viewport, int &saved, int &saved_last_col, int &saved_last_line) {

    const std::string &line = viewport.buffer.get_line(viewport.active_buffer_line_under_cursor);

    if (line.size() < saved_last_col) {
        if (saved == 0) {
            saved_last_col = viewport.active_buffer_col_under_cursor;
            saved = 1;
            viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, line.size());
        }
    }
    if (line.size() >= saved_last_col) {
        if (saved == 1) {
            viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, saved_last_col);
        }
        saved_last_col = viewport.active_buffer_col_under_cursor;
        saved = 0;
    }

    if (saved_last_line != viewport.active_buffer_line_under_cursor && saved == 1) {
        viewport.set_active_buffer_line_col_under_cursor(viewport.active_buffer_line_under_cursor, line.size());
        saved_last_line = viewport.active_buffer_line_under_cursor;
    }

    if (line.size() != viewport.active_buffer_col_under_cursor) {
        saved = 0;
        saved_last_col = viewport.active_buffer_col_under_cursor;
    }
}

bool is_integer(const std::string &str) {
    // create a stringstream from the input string
    std::stringstream ss(str);
    // declare a temporary integer variable to hold the value
    int temp;
    // try to extract an integer from the stringstream and ensure the whole string was processed
    return (ss >> temp && ss.eof());
}

int main(int argc, char *argv[]) {

    int saved_for_automatic_column_adjustment = 0;
    int saved_last_col_for_automatic_column_adjustment = 0;
    int saved_last_line_for_automatic_column_adjustment = 0;

    unsigned int windowed_screen_width_px = 700;
    unsigned int windowed_screen_height_px = 700;

    bool automatic_column_adjustment = false;
    std::string username = "tbx_user";

    // numbers must be odd to have a center
    int num_lines = 41;
    int num_cols = 101;

    bool start_in_fullscreen = false;

    Configuration::SectionKeyPairToConfigLogic section_key_to_config_logic = {
        {{"graphics", "start_in_fullscreen"},
         [&](const std::string &value) {
             if (value == "true") {
                 start_in_fullscreen = true;
             }
         }},
        {{"graphics", "windowed_screen_width_px"},
         [&](const std::string &value) {
             if (is_integer(value)) {
                 windowed_screen_width_px = std::stoi(value);
                 std::cout << "set width to " << value << std::endl;
             } else {
                 std::cout << "Error: 'windowed_screen_width_px ' is not a valid integer: " << value << std::endl;
             }
         }},
        {{"graphics", "windowed_screen_height_px"},
         [&](const std::string &value) {
             if (is_integer(value)) {
                 windowed_screen_height_px = std::stoi(value);
             } else {
                 std::cout << "Error: 'windowed_screen_height_px ' is not a valid integer: " << value << std::endl;
             }
         }},
        {{"viewport", "automatic_column_adjustment"},
         [&](const std::string &value) {
             if (value == "true") {
                 automatic_column_adjustment = true;
             }
         }},
        {{"viewport", "num_lines"},
         [&](const std::string &value) {
             if (is_integer(value)) {
                 num_lines = std::stoi(value);
             } else {
                 std::cout << "Error: 'num_lines' is not a valid integer: " << value << std::endl;
             }
         }},
        {{"viewport", "num_cols"},
         [&](const std::string &value) {
             if (is_integer(value)) {
                 num_cols = std::stoi(value);
             } else {
                 std::cout << "Error: 'num_cols' is not a valid integer: " << value << std::endl;
             }
         }},
        {{"user", "name"}, [&](const std::string &value) { username = value; }},
    };

    std::filesystem::path config_path = "~/.tbx_cfg.ini";
    Configuration config(config_path, section_key_to_config_logic);

    std::cout << username << std::endl;

    PeriodicSignal one_second_signal_for_status_bar_time_update(2);
    bool configured_rcr = false;

    RegexCommandRunner rcr;
    std::string potential_regex_command = "";
    std::cout << get_executable_path(argv) << std::endl;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("mwe_shader_cache_logs.txt", true);
    file_sink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};

    Window window;
    window.initialize_glfw_glad_and_return_window(windowed_screen_width_px, windowed_screen_height_px, "glfw window",
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
    FontAtlas font_atlas(font_info_path.string(), font_json_path.string(), font_image_path.string(),
                         windowed_screen_width_px, false, true);

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

    LineTextBuffer file_buffer;
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

                        std::string key_str = active_key.string_repr;

                        // print out the key that was just pressed
                        std::cout << "key_str:" << key_str << std::endl;

                        if (key_str == "u" && viewport.buffer.get_last_deleted_content() == "") {
                            command_bar_input = "Ain't no more history!";
                            command_bar_input_signal.toggle_state();
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
            case GLFW_KEY_CAPS_LOCK:
                current_mode = MOVE_AND_EDIT;
                mode_change_signal.toggle_state();
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
            case GLFW_KEY_TAB:
                if (current_mode == INSERT) {
                    viewport.insert_tab_at_cursor();
                }
                break;
            case GLFW_KEY_Y:
                if (current_mode == VISUAL_SELECT) {
                    std::string curr_sel = viewport.buffer.get_bounding_box_string(
                        line_where_selection_mode_started, col_where_selection_mode_started,
                        viewport.active_buffer_line_under_cursor, viewport.active_buffer_col_under_cursor);
                    glfwSetClipboardString(window.glfw_window, curr_sel.c_str());
                }
                break;
            default:
                break;
            }
        }
    };
    std::function<void(double, double)> mouse_pos_callback = [](double _, double _1) {};
    std::function<void(int, int, int)> mouse_button_callback = [](int _, int _1, int _2) {};
    GLFWLambdaCallbackManager glcm(window.glfw_window, char_callback, key_callback, mouse_pos_callback,
                                   mouse_button_callback);

    double last_time = 0.0;
    double delta_time = 0.0;
    while (!glfwWindowShouldClose(window.glfw_window)) {
        double current_time = glfwGetTime();
        delta_time = current_time - last_time;
        last_time = current_time;

        glfwGetFramebufferSize(window.glfw_window, &width, &height);

        glViewport(0, 0, width, height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        render(viewport, screen_grid, status_bar_grid, command_bar_grid, command_bar_input, monospaced_font_atlas,
               batcher, mode_change_signal, command_bar_input_signal, center_idx_x, center_idx_y, num_cols, num_lines,
               col_where_selection_mode_started, line_where_selection_mode_started, current_mode, shader_cache,
               mode_to_cursor_color, delta_time, one_second_signal_for_status_bar_time_update);

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

        run_key_logic(input_state, current_mode, mode_change_signal, viewport, line_where_selection_mode_started,
                      col_where_selection_mode_started, search_results, current_search_index, command_bar_input,
                      command_bar_input_signal, window, is_search_active, fs_browser_is_active, fs_browser_search_query,
                      searchable_files, fb, doids_for_textboxes_for_active_directory_for_later_removal, fs_browser,
                      search_results_changed_signal, selected_file_doid, currently_matched_files, rcr,
                      potential_regex_command, configured_rcr, insert_mode_signal);

        if (automatic_column_adjustment) {
            snap_to_end_of_line_while_navigating(viewport, saved_for_automatic_column_adjustment,
                                                 saved_last_col_for_automatic_column_adjustment,
                                                 saved_last_line_for_automatic_column_adjustment);
        }
        TemporalBinarySignal::process_all();
        glfwSwapBuffers(window.glfw_window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window.glfw_window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
