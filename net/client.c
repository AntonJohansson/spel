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

#define PACKET_LOG_SIZE 2048
#define OUTPUT_BUFFER_SIZE 1024

const u64 initial_server_tick_offset = 0;

bool running = true;
Time frame_start;
Time frame_end;

void inthandler(int sig) {
    (void) sig;
    running = false;
}

bool connected = false;

int main() {
    signal(SIGINT, inthandler);

    struct Player player = {0};

    struct ByteBuffer packet_log = {
        .base = malloc(PACKET_LOG_SIZE),
        .size = PACKET_LOG_SIZE,
    };
    packet_log.top = packet_log.base;

    struct ByteBuffer output_buffer = {
        .base = malloc(OUTPUT_BUFFER_SIZE),
        .size = OUTPUT_BUFFER_SIZE,
    };
    output_buffer.top = output_buffer.base;

    if (enet_initialize() != 0) {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return EXIT_FAILURE;
    }

    ENetHost *client = {0};
    client = enet_host_create(NULL, 1, 1, 0, 0);
    if (client == NULL) {
        fprintf(stderr, "An error occurred while trying to create an ENet client host.\n");
        exit(EXIT_FAILURE);
    }

    ENetAddress address = {0};
    ENetEvent event = {0};
    ENetPeer *peer = {0};
    enet_address_set_host(&address, "localhost");
    address.port = 9053;

    peer = enet_host_connect(client, &address, 2, 0);
    if (peer == NULL) {
        fprintf(stderr,
                "No available peers for initiating an ENet connection.\n");
        exit(EXIT_FAILURE);
    }
    /* Wait up to 5 seconds for the connection attempt to succeed. */
    if (enet_host_service(client, &event, 5000) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        puts("Connection to some.server.net:1234 succeeded.");
    } else {
        /* Either the 5 seconds are up or a disconnect event was */
        /* received. Reset the peer in the event the 5 seconds   */
        /* had run out without any significant event.            */
        enet_peer_reset(peer);
        puts("Connection to some.server.net:1234 failed.");
    }

    open_window();

    Time frame_start = {0},
         frame_end   = {0},
         frame_delta = {0};

    Time frame_desired = {
        .seconds = 0,
        .nanoseconds = 1000000000 / FPS,
    };

    u64 tick = 0;

    const float dt = platformTimeToNanoseconds(frame_desired);

    struct NetInput input;

    u8 adjustment_ahead = 0;
    u8 adjustment_behind = 0;
    u8 adjustment_iteration = 0;

    while (running) {
        // Begin frame
        frame_start = platformTimeCurrent();

        if (adjustment_ahead > 0) {
            adjustment_ahead--;
            goto end_frame;
        }

        client_handle_input(&input);

        packet_log.top = packet_log.base;
        // Fetch network data
        while (enet_host_service(client, &event, 0) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE: {
                // Copy packets into our own buffer
                // TODO: it would be nice if could just use the buffer that enet fetches it's
                //       packeckts into, we're kinda wasting memory here.
                append(&packet_log, event.packet->data, event.packet->dataLength);
                enet_packet_destroy(event.packet);
            } break;

            case ENET_EVENT_TYPE_DISCONNECT:
                printf("Server disconnected\n");
                break;

            case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT:
                printf("Server timeout\n");
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
            }
        }

        // Process packet queue
        u8 *p = packet_log.base;
        while (p < packet_log.top) {
            struct ServerHeader *header = (struct ServerHeader *) p;

            // TODO: reset adjustment_iteration
            if (header->adjustment != 0 && adjustment_iteration == header->adjustment_iteration) {
                printf("adjustment %d, %d, %d\n", header->adjustment, header->adjustment_iteration, adjustment_iteration);
                if (header->adjustment < 0) {
                    adjustment_ahead = -header->adjustment;
                } else {
                    adjustment_behind = header->adjustment;
                }
                ++adjustment_iteration;
                continue;
            }

            p += sizeof(struct ServerHeader);

            switch (header->type) {
            case SERVER_PACKET_NULL:
                break;
            case HELLO: {
                struct HelloFromServer *data = (struct HelloFromServer *) p;
                p += sizeof(struct HelloFromServer);

                tick = data->tick + initial_server_tick_offset;
                player.x = data->x;
                player.y = data->y;
                connected = true;
            } break;
            case AUTH: {
                struct Auth *data = (struct Auth *) p;
                p += sizeof(struct Auth);

                if (fabs(player.x - data->x) > EPSILON ||
                    fabs(player.y - data->y) > EPSILON) {
                    player.x = data->x;
                    player.y = data->y;
                    printf("Server disagreed! Forcing pos!\n");
                }
            } break;
            default:
                printf("Received unknown packet type\n");
            }

        }

        if (connected) {
            struct ClientHeader header = {
                .type = INPUT_UPDATE,
                .tick = tick,
                .adjustment_iteration = adjustment_iteration,
            };

            struct InputUpdate input_update = {
                .input = input,
            };

            output_buffer.top = output_buffer.base;
            APPEND(&output_buffer, &header);
            APPEND(&output_buffer, &input_update);

            // Send input
            ENetPacket *packet = enet_packet_create(output_buffer.base, output_buffer.top-output_buffer.base, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(peer, 0, packet);

            // Predictive move
            bool has_input = false;
            for (int i = 0; i < NET_INPUT_LAST; ++i) {
                if (input.active[i]) {
                    has_input = true;
                    break;
                }
            }
            if (has_input) {
                move(&player, &input, dt);
            }
            printf("%f, %f\n", player.x, player.y);
        }

        draw("client", &player);

        // End frame
end_frame:
        frame_end = platformTimeCurrent();
        if (adjustment_behind == 0) {
            // Only sleep remaining frame time if we aren't fast forwarding
            frame_delta = platformTimeSubtract(frame_end, frame_start);
            if (platformTimeEarlierThan(frame_delta, frame_desired)) {
                frame_delta = platformTimeSubtract(frame_desired, frame_delta);
                platformSleepNanoseconds(frame_delta);
            }
        }
        if (adjustment_behind > 0)
            adjustment_behind--;
        if (adjustment_ahead == 0)
            tick++;
    }

    // Disconnect
    bool disconnected = false;
    while (enet_host_service(client, &event, 100) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            puts("Disconnection succeeded.");
            disconnected = true;
            break;
        }
    }

    // Drop connection, since disconnection didn't successed
    if (!disconnected) {
        enet_peer_reset(peer);
    }

    enet_host_destroy(client);
    enet_deinitialize();
    close_window();
    return 0;
}
