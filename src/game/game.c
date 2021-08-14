#include <shared/api.h>
#include <shared/input.h>

void update(struct game_memory *memory, struct frame_input *input, struct render_commands *frame) {
    struct render_entry_quad *quad = PUSH_RENDER_ENTRY(frame, render_entry_quad);
    quad->lower_left_corner = V2(0,0);
    quad->upper_right_corner = V2(0.5,0.5);

    if (input->active[INPUT_MOVE_LEFT]) {
        memory->platform.log(LOG_INFO, "move left");
    } else if (input->active[INPUT_MOVE_RIGHT]) {
        memory->platform.log(LOG_INFO, "move right");
    } else if (input->active[INPUT_JUMP]) {
        memory->platform.log(LOG_INFO, "jump");
    }
}
