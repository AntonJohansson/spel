#pragma once

#include <stdbool.h>

typedef enum InputType {
    INPUT_EMPTY = 0,
    INPUT_MOVE_LEFT,
    INPUT_MOVE_RIGHT,
    INPUT_MOVE_UP,
    INPUT_MOVE_DOWN,
    INPUT_JUMP,

    INPUT_LAST,
} InputType;

/* 
 * Start/stop currently have to be
 * separate since we only handle "holding"
 * actions
 */
typedef enum DebugInputType {
    DEBUG_INPUT_EMPTY = 0,
    DEBUG_INPUT_RECORD_START,
    DEBUG_INPUT_RECORD_STOP,
    DEBUG_INPUT_REPLAY_START,
    DEBUG_INPUT_REPLAY_STOP,

    DEBUG_INPUT_LAST
} DebugInputType;

typedef struct Input {
    bool active[INPUT_LAST];
} Input;

typedef struct DebugInput {
    bool active[DEBUG_INPUT_LAST];
} DebugInput;
