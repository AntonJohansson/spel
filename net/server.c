#define ENET_IMPLEMENTATION
#include "enet.h"
#include "api.h"
#include "draw.h"
#include "../src/loader/platform/platform.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <math.h>

#define FPS 20

#define MAX_CLIENTS 32

#define PACKET_LOG_SIZE 2048
#define OUTPUT_BUFFER_SIZE 1024

bool running = true;

void inthandler(int sig) {
    (void) sig;
    running = false;
}

int main() {
    signal(SIGINT, inthandler);

    struct Player player = {
        .x = 400.0f,
        .y = 300.0f,
    };

    struct ByteBuffer output_buffer = {
        .base = malloc(OUTPUT_BUFFER_SIZE),
        .size = OUTPUT_BUFFER_SIZE,
    };
    output_buffer.top = output_buffer.base;

    if (enet_initialize() != 0) {
        printf("An error occurred while initializing ENet.\n");
        return 1;
    }

    ENetAddress address = {0};

    address.host = ENET_HOST_ANY;
    address.port = 9053;

    /* create a server */
    ENetHost *server = enet_host_create(&address, MAX_CLIENTS, 1, 0, 0);

    if (server == NULL) {
        printf("An error occurred while trying to create an ENet server host.\n");
        return 1;
    }

    printf("Started a server...\n");

    open_window();

    ENetEvent event = {0};

    Time frame_start = {0},
         frame_end   = {0},
         frame_delta = {0};

    Time frame_desired = {
        .seconds = 0,
        .nanoseconds = 1000000000 / FPS,
    };

    u64 tick = 0;

    const float dt = platformTimeToNanoseconds(frame_desired);

    /* Wait up to 1000 milliseconds for an event. (WARNING: blocking) */
    while (running) {
        //printf("tick %llu\n", tick);
        // Begin frame
        frame_start = platformTimeCurrent();

        // Handle network
        while (enet_host_service(server, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",  event.peer->address.host, event.peer->address.port);
                struct ServerHeader header = {
                    .type = HELLO,
                };

                struct HelloFromServer hello = {
                    .tick = tick,
                    .x = player.x,
                    .y = player.y,
                };

                output_buffer.top = output_buffer.base;
                APPEND(&output_buffer, &header);
                APPEND(&output_buffer, &hello);

                ENetPacket *packet = enet_packet_create(output_buffer.base, output_buffer.top-output_buffer.base, ENET_PACKET_FLAG_RELIABLE);
                enet_peer_send(event.peer, 0, packet);

                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                char *p = event.packet->data;
                struct ClientHeader *header = (struct ClientHeader *) p;
                p += sizeof(struct ClientHeader);

                switch (header->type) {
                case INPUT_UPDATE: {
                    i8 adjustment = 0;
                    i64 diff = (i64) tick + (2-1) - (i64) header->tick;
                    if (diff < INT8_MIN || diff > INT8_MAX) {
                        printf("tick diff outside range of adjustment variable!\n");
                        // TODO: what do?
                        break;
                    }
                    if (diff < -(2-1) || diff > 0) {
                        // Need adjustment
                        adjustment = (i8) diff;
                    }

                    struct ServerHeader response_header = {
                        .type = SERVER_PACKET_NULL,
                        .tick = tick, // not used by client
                        .adjustment = adjustment,
                        .adjustment_iteration = header->adjustment_iteration,
                    };

                    if (header->tick >= tick) {
                        response_header.type = AUTH;
                        struct InputUpdate *input_update = (struct InputUpdate *) p;

                        if (diff < -(2-1)) {
                            printf("Allowing packet, too late: tick %llu, should be >= %llu\n", header->tick, tick);
                        } else {
                            printf("Allowing packet, tick %llu, %llu\n", header->tick, tick);
                        }

                        // Currently applying all packets immediately
                        move(&player, &input_update->input, dt);

                        struct Auth auth = {
                            .x = player.x,
                            .y = player.y,
                        };

                        output_buffer.top = output_buffer.base;
                        APPEND(&output_buffer, &response_header);
                        APPEND(&output_buffer, &auth);

                        ENetPacket *packet = enet_packet_create(output_buffer.base, output_buffer.top-output_buffer.base, ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    } else {
                        // Here response_header->type is PACKET_NULL
                        printf("Dropping packet, too early: tick %llu, should be >= %llu\n", header->tick, tick);

                        output_buffer.top = output_buffer.base;
                        APPEND(&output_buffer, &response_header);

                        ENetPacket *packet = enet_packet_create(output_buffer.base, output_buffer.top-output_buffer.base, ENET_PACKET_FLAG_RELIABLE);
                        enet_peer_send(event.peer, 0, packet);
                    }

                } break;
                default:
                    printf("Received unknown packet type\n");
                }
                enet_packet_destroy(event.packet);
            } break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%s disconnected.\n", event.peer->data);
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("%s disconnected due to timeout.\n", event.peer->data);
                event.peer->data = NULL;
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }

        draw("server", &player);

        // End frame
        frame_end = platformTimeCurrent();
        frame_delta = platformTimeSubtract(frame_end, frame_start);
        if (platformTimeEarlierThan(frame_delta, frame_desired)) {
            frame_delta = platformTimeSubtract(frame_desired, frame_delta);
            platformSleepNanoseconds(frame_delta);
        }
        tick++;
    }

    enet_host_destroy(server);
    enet_deinitialize();
    close_window();
    return 0;
}
