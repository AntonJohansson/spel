#include "draw.h"
#include <string.h>
#include <raylib.h>

#define WIDTH 800
#define HEIGHT 600

void open_window() {
    InitWindow(WIDTH, HEIGHT, "wow");
}

void draw(const char *str, struct Player *player) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText(str, 10, 10, 20, BLACK);
    DrawCircle(player->x, player->y, 10.0f, RED);
    EndDrawing();
}

void close_window() {
    CloseWindow();
}

void client_handle_input(struct NetInput *input) {
    memset(input->active, NET_INPUT_NULL, sizeof(NET_INPUT_NULL)*NET_INPUT_LAST);
    if (IsKeyDown(KEY_W))
        input->active[NET_INPUT_MOVE_UP] = true;
    if (IsKeyDown(KEY_A))
        input->active[NET_INPUT_MOVE_LEFT] = true;
    if (IsKeyDown(KEY_S))
        input->active[NET_INPUT_MOVE_DOWN] = true;
    if (IsKeyDown(KEY_D))
        input->active[NET_INPUT_MOVE_RIGHT] = true;
}
