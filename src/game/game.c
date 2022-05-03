#include <shared/api.h>
#include <shared/types.h>
#include <shared/input.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void load_image(GameMemory *memory, const char *file, Image *image) {
    image->pixels = stbi_load(file, &image->width, &image->height, &image->channels, STBI_rgb_alpha);
    if (!image->pixels) {
        memory->platform.log(LOG_ERROR, "stb_image: failed to load %s!", file);
        return;
    }
    // WARNING: not freeing
    //stbi_image_free(pixels);
}

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

    pushQuad(frame, VEC2(0,0), VEC2(1.0f, 0.05f), convertHSLToRGB(memory->col));
    pushQuad(frame, memory->pos, VEC2(0.25f, 0.2f), convertHSLToRGB(memory->col));
}
