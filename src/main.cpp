// ABOUTODO
// need to get change word in there and so on

#include <algorithm>
#include <fmt/core.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics/window/window.hpp"
#include "graphics/shader_cache/shader_cache.hpp"
#include "graphics/batcher/generated/batcher.hpp"
#include "graphics/vertex_geometry/vertex_geometry.hpp"
#include "graphics/texture_atlas/texture_atlas.hpp"
#include "graphics/viewport/viewport.hpp"

#include "utility/input_state/input_state.hpp"
#include "utility/glfw_lambda_callback_manager/glfw_lambda_callback_manager.hpp"
#include "utility/limited_vector/limited_vector.hpp"
#include "utility/temporal_binary_signal/temporal_binary_signal.hpp"
#include "utility/text_buffer/text_buffer.hpp"
#include "utility/input_state/input_state.hpp"

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
                                             TemporalBinarySignal &mode_change_signal, GLFWwindow *window) {
        // Command must be more than one character to proceed

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

unsigned int SCREEN_WIDTH = 1366;
unsigned int SCREEN_HEIGHT = 768;

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

    batcher.absolute_position_textured_shader_batcher.draw_everything();
    batcher.absolute_position_with_solid_color_shader_batcher.draw_everything();
}

int main(int argc, char *argv[]) {

    std::cout << get_executable_path(argv) << std::endl;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::debug);
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("mwe_shader_cache_logs.txt", true);
    file_sink->set_level(spdlog::level::info);
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};

    GLFWwindow *window =
        initialize_glfw_glad_and_return_window(SCREEN_WIDTH, SCREEN_HEIGHT, "glfw window", true, false, false);

    std::vector<ShaderType> requested_shaders = {ShaderType::ABSOLUTE_POSITION_TEXTURED,
                                                 ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR};

    ShaderCache shader_cache(requested_shaders, sinks);
    Batcher batcher(shader_cache);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    InputState input_state;

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
    };

    // Define the key callback
    std::function<void(int, int, int, int)> key_callback = [&](int key, int scancode, int action, int mods) {
        // these events happen once when the key is pressed down, aka its non-repeating; a one time event

        if (action == GLFW_PRESS || action == GLFW_RELEASE) {
            Key active_key = input_state.glfw_code_to_key.at(key);
            bool is_pressed = (action == GLFW_PRESS);
            input_state.key_to_state.at(active_key).set_signal(is_pressed);

            if (current_mode == MOVE_AND_EDIT) {
                if (action == GLFW_PRESS) {
                    auto key_type = get_key_type(active_key);
                    if (key_type == KeyType::ALPHA or key_type == KeyType::NUMERIC) {
                        if (mods & GLFW_MOD_SHIFT) {
                            active_key = get_shifted_key(active_key);
                        }
                        std::string key_str = input_state.key_to_key_string.at(active_key);
                        // maybe this can be generalized to unary, binary etc type commands
                        std::vector<std::string> potential_automatic_command_prefixes = {"d", "c", "y", "f",
                                                                                         "F", "t", "T"};

                        bool command_started_or_continuing =
                            starts_with_any_prefix(key_str, potential_automatic_command_prefixes) or
                            move_and_edit_arcr.command_started;
                        bool control_not_pressed = input_state.key_to_state.at(Key::LEFT_CONTROL).is_off();
                        if (command_started_or_continuing and control_not_pressed) {
                            potential_automatic_command += key_str;
                            if (potential_automatic_command.length() > 3) {
                                potential_automatic_command = potential_automatic_command.substr(1);
                            }
                            std::cout << potential_automatic_command << std::endl;
                        }
                        bool command_was_run = move_and_edit_arcr.potentially_run_normal_mode_command(
                            potential_automatic_command, viewport, current_mode, mode_change_signal, window);
                        if (command_was_run) {
                            potential_automatic_command = "";
                        }
                    }
                    if (active_key == Key::ESCAPE or active_key == Key::CAPS_LOCK) {
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
                if (current_mode == INSERT) {
                    viewport.backspace_at_active_position();
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

        // not sure why this has to go here right now but it doesn't update if it comes after mofifying viewport.

        viewport.tick();
        if (input_state.key_to_state.at(Key::ESCAPE).is_just_on()) {
            current_mode = MOVE_AND_EDIT;
            mode_change_signal.toggle_state();
        }

        std::function<void()> shared_m_and_e_and_visual_selection_logic = [&]() {
            if (input_state.key_to_state.at(Key::J).is_just_on()) {
                viewport.scroll_down();
            }

            if (input_state.key_to_state.at(Key::K).is_just_on()) {
                viewport.scroll_up();
            }

            if (input_state.key_to_state.at(Key::H).is_just_on()) {
                viewport.scroll_left();
            }

            if (input_state.key_to_state.at(Key::L).is_just_on()) {
                viewport.scroll_right();
            }

            if (input_state.key_to_state.at(Key::W).is_just_on()) {
                viewport.move_cursor_forward_by_word();
            }

            if (input_state.key_to_state.at(Key::E).is_just_on()) {
                viewport.move_cursor_forward_until_end_of_word();
            }

            if (input_state.key_to_state.at(Key::LEFT_SHIFT).is_just_on()) {
                viewport.move_cursor_backward_until_start_of_word();
            } else {
                if (input_state.key_to_state.at(Key::B).is_just_on()) {
                    viewport.move_cursor_backward_by_word();
                }
            }
        };

        // we only run our commands if there is not an active command occuring in move and edit mode
        if (not move_and_edit_arcr.command_started) {
            if (current_mode == MOVE_AND_EDIT) {
                shared_m_and_e_and_visual_selection_logic();
                if (input_state.key_to_state.at(Key::LEFT_CONTROL).is_on()) {
                    if (input_state.key_to_state.at(Key::U).is_just_on()) {
                        viewport.scroll(-5, 0);
                    }
                    if (input_state.key_to_state.at(Key::D).is_just_on()) {
                        viewport.scroll(5, 0);
                    }
                }
                if (input_state.key_to_state.at(Key::O).is_just_on()) {
                    viewport.create_new_line_at_cursor_and_scroll_down();
                }
                if (input_state.key_to_state.at(Key::V).is_just_on()) {
                    current_mode = VISUAL_SELECT;
                    mode_change_signal.toggle_state();
                    line_where_selection_mode_started = viewport.active_buffer_line_under_cursor;
                    col_where_selection_mode_started = viewport.active_buffer_col_under_cursor;
                }
                if (input_state.key_to_state.at(Key::U).is_just_on()) {
                    if (current_mode == MOVE_AND_EDIT) {
                        viewport.buffer.undo();
                    }
                }
                if (input_state.key_to_state.at(Key::R).is_just_on()) {
                    if (current_mode == MOVE_AND_EDIT) {
                        viewport.buffer.redo();
                    }
                }
                if (input_state.key_to_state.at(Key::LEFT_SHIFT).is_on()) {
                    if (input_state.key_to_state.at(Key::N).is_just_on()) {
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
                    if (input_state.key_to_state.at(Key::N).is_just_on()) {
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
                if (input_state.key_to_state.at(Key::LEFT_SHIFT).is_on()) {
                    if (input_state.key_to_state.at(Key::SEMICOLON).is_just_on()) {
                        current_mode = COMMAND;
                        command_bar_input = ":";
                        command_bar_input_signal.toggle_state();
                        mode_change_signal.toggle_state();
                    }
                }
                if (input_state.key_to_state.at(Key::SLASH).is_just_on()) {
                    current_mode = COMMAND;
                    command_bar_input = "/";
                    command_bar_input_signal.toggle_state();
                    mode_change_signal.toggle_state();
                }

            } else if (current_mode == COMMAND) {
                if (input_state.key_to_state.at(Key::ESCAPE).is_just_on()) {
                    command_bar_input = "";
                    command_bar_input_signal.toggle_state();
                    current_mode = MOVE_AND_EDIT;
                    mode_change_signal.toggle_state();
                }
                if (input_state.key_to_state.at(Key::ENTER).is_just_on()) {
                    if (command_bar_input == ":w") {
                        viewport.buffer.save_file();
                    }
                    if (command_bar_input == ":q") {
                        glfwSetWindowShouldClose(window, true);
                    }
                    if (command_bar_input.front() == '/') {
                        // Forward search command
                        std::string search_request = command_bar_input.substr(1); // remove the "/"
                        search_results = viewport.buffer.find_forward_matches(viewport.active_buffer_line_under_cursor,
                                                                              viewport.active_buffer_col_under_cursor,
                                                                              search_request);
                        if (!search_results.empty()) {
                            is_search_active = true;
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
            if (input_state.key_to_state.at(Key::ENTER).is_just_on()) {
                viewport.create_new_line_at_cursor_and_scroll_down();
            }
        } else if (current_mode == VISUAL_SELECT) {
            shared_m_and_e_and_visual_selection_logic();
        }

        TemporalBinarySignal::process_all();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
