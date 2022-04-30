#include <shared/api.h>
#include <shared/types.h>
#include <shared/input.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <shared/quicksort.h>
#include <shared/pack_rectangles.h>

static void load_image(GameMemory *memory, const char *file, Image *image) {
    image->pixels = stbi_load(file, &image->width, &image->height, &image->channels, STBI_rgb_alpha);
    if (!image->pixels) {
        memory->platform.log(LOG_ERROR, "stb_image: failed to load %s!", file);
        return;
    }
    // WARNING: not freeing
    //stbi_image_free(pixels);
}

#define NUM_CHARS 128
PackRect rects[NUM_CHARS] = {0};

static void load_font_atlas(GameMemory *memory, Image *image) {
    if (FT_Init_FreeType(&memory->ft)) {
        memory->platform.log(LOG_ERROR, "FreeType: Could not init!");
        return;
    }

    u8 *font = NULL;
    u64 file_size = 0;
    memory->platform.read_file_to_buffer("res/fonts/lmroman5-regular.otf", &font, &file_size);

    FT_Open_Args args = {
        .flags = FT_OPEN_MEMORY,
        .memory_base = font,
        .memory_size = file_size,
        .pathname = NULL,
        .stream = 0,
        .driver = NULL,
        .num_params = 0,
        .params = NULL,
    };

    if (FT_Open_Face(memory->ft, &args, 0, &memory->face)) {
        memory->platform.log(LOG_ERROR, "FreeType: Could not open face!");
        return;
    }

    memory->platform.log(LOG_INFO, "num faces %ld", memory->face->num_faces);
    memory->platform.log(LOG_INFO, "num glyphs %ld", memory->face->num_glyphs);
    memory->platform.log(LOG_INFO, "family name %s", memory->face->family_name);
    memory->platform.log(LOG_INFO, "style name %s", memory->face->style_name);
    memory->platform.log(LOG_INFO, "num fixed sizes %d", memory->face->num_fixed_sizes);
    memory->platform.log(LOG_INFO, "num fixed sizes %d", memory->face->size);

    memory->platform.free_memory(font);

    memory->font_size = 120;
    FT_Set_Pixel_Sizes(memory->face, 0, memory->font_size);
    memory->platform.log(LOG_INFO, "num fixed sizes %d", memory->face->size);

    /* Load font rects */
    for (u8 i = 0; i < NUM_CHARS; ++i) {
        FT_UInt glyph_index = FT_Get_Char_Index(memory->face, i);
        if (FT_Load_Char(memory->face, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY)) {
            memory->platform.log(LOG_ERROR, "FreeType: Could not load glyph for character: %c", i);
            return;
        }
        //if (rects[i].width > 0 && rects[i].height > 0)
        rects[i].width = memory->face->glyph->bitmap.width;
        rects[i].height = memory->face->glyph->bitmap.rows;
        rects[i].user_id = glyph_index;
    }

    const u32 pack_width = sqrt(memory->font_size*memory->font_size*NUM_CHARS);
    const u32 pack_height = pack_width;
    PackNode nodes[pack_width+2];
    PackContext pack_context = {
        .width = pack_width,
        .height = pack_height,
        .num_nodes = pack_width + 2,
        .nodes = nodes,
    };
    pack_rectangles(&pack_context, rects, NUM_CHARS);

    u8 pack_pixels[pack_width*pack_height];
    memset(pack_pixels, 0, pack_width*pack_height*sizeof(u8));
    for (u8 i = 0; i < NUM_CHARS; ++i) {
        u8 index = rects[i].user_id;
        if (FT_Load_Char(memory->face, index, FT_LOAD_RENDER)) {
            memory->platform.log(LOG_ERROR, "FreeType: Could not load glyph for character: %c", i);
            return;
        }
        if (rects[i].width > 0 && rects[i].height > 0) {
            for (u32 y = 0; y < rects[i].height; ++y) {
                for (u32 x = 0; x < rects[i].width; ++x) {
                    pack_pixels[(rects[i].y + y)*pack_width + rects[i].x + x] = memory->face->glyph->bitmap.buffer[y*rects[i].width + x];
                }
            }
        }
    }

    image->pixels = memory->platform.allocate_memory(pack_width*pack_height*sizeof(u8));
    image->width = pack_width;
    image->height = pack_height;
    image->channels = 1;

    memcpy(image->pixels, pack_pixels, pack_width*pack_height*sizeof(u8));
}

void drawText(GameMemory *memory, RenderCommands *frame, const char *str) {

    float x = -0.75f;
    for (const char *p = str; *p; ++p) {
        FT_UInt glyph_index = FT_Get_Char_Index(memory->face, *p);
        if (FT_Load_Char(memory->face, glyph_index, FT_LOAD_BITMAP_METRICS_ONLY)) {
            memory->platform.log(LOG_ERROR, "FreeType: Could not load glyph for character: %c", *p);
            return;
        }

        PackRect *rect = NULL;
        for (u32 i = 0; i < NUM_CHARS; ++i) {
            if (rects[i].user_id == glyph_index) {
                rect = &rects[i];
                break;
            }
        }

        if (rect) {
            //pushAtlasQuad(frame, VEC2(x,-0.5), VEC2(2.0f*rect->width/800.0f, 2.0f*rect->height/600.0f), memory->font_atlas,
            //              VEC2(rect->x/(f32)memory->font_atlas.width, rect->y/(f32)memory->font_atlas.height),
            //              //VEC2(rect->width/(f32)memory->font_atlas.width, rect->height/(f32)memory->font_atlas.height)
            //              VEC2(0.1,0.1)
            //              );
            x += 2.0*(float)(memory->face->glyph->advance.x >> 6)/800.0f;
        }
    }
}

void update(f32 t, GameMemory *memory, Input *input, RenderCommands *frame) {
    if (!memory->font_atlas_loaded) {
        load_font_atlas(memory, &memory->font_atlas);
        memory->font_atlas_loaded = true;
    }
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

    pushTexturedQuad(frame, VEC2(0,0), VEC2(1, 1), memory->font_atlas);
    pushQuad(frame, VEC2(0,0), VEC2(1.0f, 0.05f), convertHSLToRGB(memory->col));
    pushQuad(frame, memory->pos, VEC2(0.25f, 0.2f), convertHSLToRGB(memory->col));

    drawText(memory, frame, "wow");
}
