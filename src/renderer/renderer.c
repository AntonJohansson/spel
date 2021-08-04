#include <shared/api.h>

struct render_commands *begin_frame(struct renderer *r) {
    r->cmds.memory_base = &r->memory[0];
    r->cmds.memory_top  = 0;
    r->cmds.memory_size = RENDERER_MEMORY_SIZE;
    return &r->cmds;
}

void end_frame(struct renderer *r, struct render_commands *cmds) {
    struct render_entry_header *header;

    header = (struct render_entry_header *) cmds->memory_base;
    switch (header->type) {
        case ENTRY_TYPE_render_entry_quad: {
            //r->platform.log(LOG_INFO, "Hello from quad\n");
            break;
        }
    };
}
