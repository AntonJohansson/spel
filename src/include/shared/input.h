#pragma once

#include <stdbool.h>

enum input_type {
    INPUT_EMPTY = 0,
    INPUT_MOVE_LEFT,
    INPUT_MOVE_RIGHT,
    INPUT_JUMP,
    INPUT_LAST
};

struct frame_input {
    bool active[INPUT_LAST];
};
