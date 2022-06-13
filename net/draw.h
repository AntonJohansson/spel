#pragma once

#include "api.h"

void open_window();
void draw(const char *str, struct Player *player);
void close_window();

void client_handle_input(struct NetInput *input);
