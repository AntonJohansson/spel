#include <shared/api.h>
#include <shared/types.h>
#include <shared/input.h>

void update(f32 t, GameMemory *memory, Input *input, RenderCommands *frame) {
    memory->col.h += 100.f*t;
    memory->col.s = 0.9f;
    memory->col.l = 0.2f;
    wrapHSL(&memory->col);

    if (input->active[INPUT_MOVE_LEFT]) {
        memory->pos.x -= 0.01f;
    }
    if (input->active[INPUT_MOVE_RIGHT]) {
        memory->pos.x += 0.01f;
    }
    if (input->active[INPUT_MOVE_UP]) {
        memory->pos.y -= 0.01f;
    }
    if (input->active[INPUT_MOVE_DOWN]) {
        memory->pos.y += 0.01f;
    }

    pushQuad(frame, VEC2(0.25f,0.25f), VEC2(0.5f, 0.5f), convertHSLToRGB(memory->col));
    pushQuad(frame, memory->pos, VEC2(0.5f, 0.5f), convertHSLToRGB(memory->col));
}
