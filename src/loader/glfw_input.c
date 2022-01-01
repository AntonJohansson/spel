
static void mouse_move_callback(GLFWwindow *window, double x, double y) {
    static double last_normalized_x = 0.0;
    static double last_normalized_y = 0.0;

    int w = 0, h= 0;
    glfwGetWindowSize(window, &w, &h);

    double normalized_x = 2.0 * x / (double) w - 1.0;
    double normalized_y = 1.0 - 2.0 * y / h;

    double dx = normalized_x - last_normalized_x;
    double dy = normalized_y - last_normalized_y;
}

static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
}

static void mouse_scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        InputType type = global_key_map[key];
        InputType debug_type = global_debug_key_map[key];
        if (type != INPUT_EMPTY) {
            global_frame_input.active[type] = true;
        }
        if (debug_type != DEBUG_INPUT_EMPTY) {
            global_debug_frame_input.active[debug_type] = true;
        }
    } else if (action == GLFW_RELEASE) {
        InputType type = global_key_map[key];
        InputType debug_type = global_debug_key_map[key];
        if (type != INPUT_EMPTY) {
            global_frame_input.active[type] = false;
        }
        if (debug_type != INPUT_EMPTY) {
            global_debug_frame_input.active[debug_type] = false;
        }
    }
}

static void character_callback(GLFWwindow *window, unsigned int codeppoint) {
}

static void window_size_callback(GLFWwindow *window, int width, int height) {
}

static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
}

static void set_glfw_input_callbacks(GLFWwindow *window) {
    glfwSetCursorPosCallback(window, mouse_move_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, mouse_scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetCharCallback(window, character_callback);
    glfwSetWindowSizeCallback(window, window_size_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
}
