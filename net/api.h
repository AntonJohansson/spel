#pragma once

#include <shared/types.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>

#define EPSILON (1e-4)

struct Player {
    float x;
    float y;
};

enum NetInputType {
    NET_INPUT_NULL,
    NET_INPUT_MOVE_LEFT,
    NET_INPUT_MOVE_RIGHT,
    NET_INPUT_MOVE_UP,
    NET_INPUT_MOVE_DOWN,
    NET_INPUT_LAST,
};

struct NetInput {
    bool active[NET_INPUT_LAST];
};

enum ServerPacketType {
    SERVER_PACKET_NULL,
    HELLO,
    AUTH,
};

enum ClientPacketType {
    CLIENT_PACKET_NULL,
    INPUT_UPDATE,
};

struct ServerHeader {
    enum ServerPacketType type;
    u64 tick; // only used client -> server
    i8 adjustment;
    u8 adjustment_iteration;
};

struct ClientHeader {
    enum ClientPacketType type;
    u64 tick;
    u8 adjustment_iteration;
};

struct HelloFromServer {
    u64 tick;
    float x, y;
};

struct InputUpdate {
    struct NetInput input;
};

struct Auth {
    float x, y;
};

struct ByteBuffer {
    u8 *base;
    u8 *top;
    size_t size;
};

#define APPEND(buffer, data) \
    append(buffer, data, sizeof(*data))

static inline void append(struct ByteBuffer *buffer, void *data, size_t size) {
    assert(buffer->top + size <= buffer->base + buffer->size);
    memcpy(buffer->top, data, size);
    buffer->top += size;
}

static inline void move(struct Player *p, struct NetInput *input, const float dt) {
    const float move_speed = 10.0f;
    float dir_x = 1.0f*(float) input->active[NET_INPUT_MOVE_RIGHT] - 1.0f*(float) input->active[NET_INPUT_MOVE_LEFT];
    float dir_y = 1.0f*(float) input->active[NET_INPUT_MOVE_DOWN]  - 1.0f*(float) input->active[NET_INPUT_MOVE_UP];
    float length2 = dir_x*dir_x + dir_y*dir_y;
    if (length2 > 0) {
        float length = sqrtf(length2);
        dir_x *= move_speed/length;
        dir_y *= move_speed/length;

        p->x += dir_x;
        p->y += dir_y;
    }
}
