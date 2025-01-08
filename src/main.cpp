#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "graphics/window/window.hpp"
#include "graphics/shader_cache/shader_cache.hpp"
#include "graphics/batcher/generated/batcher.hpp"
#include "graphics/vertex_geometry/vertex_geometry.hpp"
#include "graphics/texture_atlas/texture_atlas.hpp"
#include "graphics/viewport/viewport.hpp"

#include "utility/glfw_lambda_callback_manager/glfw_lambda_callback_manager.hpp"
#include "utility/temporal_binary_signal/temporal_binary_signal.hpp"
#include "utility/text_buffer/text_buffer.hpp"

#include <cstdio>
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>

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

static void error_callback(int error, const char *description) { fprintf(stderr, "Error: %s\n", description); }

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
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

int main(int argc, char *argv[]) {

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

    // TODO add a function which can create a rectangle from four points with assersts that it creates a rectangle

    Grid screen_grid(num_lines, num_cols);

    int center_idx_x = num_cols / 2;
    int center_idx_y = num_lines / 2;

    std::unordered_map<char, IndexedVertices> font;

    glm::vec2 active_visible_screen_position;

    // Check if the user provided a file argument
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1; // Return an error code if no argument is provided
    }

    std::string filename = argv[1]; // Get the file name from the first argument

    LineTextBuffer buffer;
    if (!buffer.load_file(filename)) {
        return 1; // Return an error code if the file couldn't be loaded
    }

    EditorMode current_mode = MOVE_AND_EDIT;
    TemporalBinarySignal mode_change_signal;
    Viewport viewport(buffer, center_idx_y, center_idx_x);

    TemporalBinarySignal insert_mode_signal;

    std::function<void(unsigned int)> char_callback = [&](unsigned int character_code) {
        if (current_mode == INSERT) {

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
    };

    // Define the key callback
    std::function<void(int, int, int, int)> key_callback = [&](int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS || action == GLFW_REPEAT) {
            switch (key) {
                // this case switch is so BS, need to use the frag-z thing for handling keyboard input, take a look at
                // that again.
            case GLFW_KEY_V:
                if (current_mode == MOVE_AND_EDIT) {
                    current_mode = VISUAL_SELECT;
                    mode_change_signal.toggle_state();
                    line_where_selection_mode_started = viewport.active_buffer_line_under_cursor;
                    col_where_selection_mode_started = viewport.active_buffer_col_under_cursor;
                }
                break;
            case GLFW_KEY_I:
                if (current_mode == MOVE_AND_EDIT) {
                    current_mode = INSERT;
                    insert_mode_signal.toggle_state();
                    mode_change_signal.toggle_state();
                }
                break;
            case GLFW_KEY_C:
                if (current_mode == MOVE_AND_EDIT) {
                    current_mode = COMMAND;
                    mode_change_signal.toggle_state();
                }
                break;
            case GLFW_KEY_ESCAPE:
                current_mode = MOVE_AND_EDIT;
                mode_change_signal.toggle_state();
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
                    current_mode = MOVE_AND_EDIT;
                    mode_change_signal.toggle_state();
                }
                break;
            case GLFW_KEY_O:
                if (current_mode == MOVE_AND_EDIT) {
                    viewport.create_new_line_at_cursor_and_scroll_down();
                    current_mode = INSERT;
		    mode_change_signal.toggle_state();
                }
                break;
            case GLFW_KEY_BACKSPACE:
                if (current_mode == INSERT) {
                    viewport.backspace_at_active_position();
                }
                break;
            case GLFW_KEY_F:
                if (current_mode == COMMAND) {
                    viewport.buffer.save_file();
                }
                break;
            case GLFW_KEY_Q:
                if (current_mode == COMMAND) {
                    glfwSetWindowShouldClose(window, true);
                }
                break;
            case GLFW_KEY_J: // Move viewport up
                if (current_mode == MOVE_AND_EDIT or current_mode == VISUAL_SELECT) {
                    viewport.scroll_down();
                }
                break;
            case GLFW_KEY_K: // Move viewport down
                if (current_mode == MOVE_AND_EDIT or current_mode == VISUAL_SELECT) {
                    viewport.scroll_up();
                }
                break;
            case GLFW_KEY_L: // Move viewport left
                if (current_mode == MOVE_AND_EDIT or current_mode == VISUAL_SELECT) {
                    viewport.scroll_left();
                }
                break;
            case GLFW_KEY_SEMICOLON: // Move viewport right
                if (current_mode == MOVE_AND_EDIT or current_mode == VISUAL_SELECT) {
                    viewport.scroll_right();
                }
                break;
            case GLFW_KEY_W:
                if (mods & GLFW_MOD_SHIFT) {
                    // Shift + W: Move cursor backward by a word
                    viewport.move_cursor_backward_by_word();
                } else {
                    // W: Move cursor forward by a word
                    viewport.move_cursor_forward_by_word();
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

        bool should_replace =
            viewport.moved_signal.has_just_changed() or viewport.buffer.edit_signal.has_just_changed();

        int unique_idx = 0;
        for (int line = 0; line < screen_grid.rows; line++) {
            for (int col = 0; col < screen_grid.cols; col++) {

                auto cell_rect = screen_grid.get_at(col, line);
                IndexedVertices cell_ivs = cell_rect.get_ivs();
                std::string cell_char(1, viewport.get_symbol_at(line, col));
                auto cell_char_tcs = monospaced_font_atlas.get_texture_coordinates_of_sub_texture(cell_char);
                // because the texture has the font inside the cell.
                adjust_uv_coordinates_in_place(cell_char_tcs, 0.017, 0.045, 0.01);
                batcher.absolute_position_textured_shader_batcher.queue_draw(
                    unique_idx, cell_ivs.indices, cell_ivs.vertices, cell_char_tcs, should_replace);
                unique_idx++;
            }
        }

	if (should_replace) {
	    auto selected_color = mode_to_cursor_color[current_mode];
	    shader_cache.set_uniform(ShaderType::ABSOLUTE_POSITION_WITH_SOLID_COLOR, ShaderUniformVariable::RGBA_COLOR,
				     selected_color);
	}

        if (current_mode == VISUAL_SELECT) {
            int visual_col_delta = -(viewport.active_buffer_col_under_cursor - col_where_selection_mode_started);
            int visual_line_delta = -(viewport.active_buffer_line_under_cursor - line_where_selection_mode_started);

            std::vector<Rectangle> visually_selected_rectangles = screen_grid.get_rectangles_in_bounding_box(
                center_idx_y + visual_line_delta, center_idx_x + visual_col_delta, center_idx_y, center_idx_x);

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

        TemporalBinarySignal::process_all();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
