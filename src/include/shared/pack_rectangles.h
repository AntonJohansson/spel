#pragma once

#include <shared/quicksort.h>
#include <assert.h>

typedef struct PackRect {
    u32 x, y;
    u32 width, height;
    u32 user_id;
} PackRect;

typedef struct PackNode {
    u32 x, y;
    struct PackNode *next;
} PackNode;

typedef struct PackContext {
    u32 width;
    u32 height;
    u64 num_nodes;
    PackNode *nodes;
    PackNode *active_list;
    PackNode *free_list;
} PackContext;

static inline void find_min_y(PackContext *context, PackNode *node, u32 x0, u32 width, u32 *out_y) {
    u32 min_y = 0;
    u32 x1 = x0 + width;
    while (node->x < x1) {
        if (node->y > min_y) {
            min_y = node->y;
        }
        node = node->next;
    }
    *out_y = min_y;
}

static inline PackNode **find_best_pack_pos(PackContext *context, u32 width, u32 height, u32 *out_x, u32 *out_y) {
    u32 best_y = 1 << 30;

    if (width > context->width || height > context->height) {
        return NULL;
    }

    PackNode *node = context->active_list;
    PackNode **prev = &context->active_list;
    PackNode **best = NULL;
    while (node->x + width <= context->width) {
        u32 y; 
        find_min_y(context, node, node->x, width, &y);
        if (y < best_y) {
            best_y = y;
            best = prev;
        }
        prev = &node->next;
        node = node->next;
    }

    u32 best_x = (best) ? (*best)->x : 0;
    *out_x = best_x;
    *out_y = best_y;

    return best;
}

static bool compare_pack_rects(const void *a, const void *b) {
    return ((PackRect *)a)->height > ((PackRect *)b)->height;
}

static inline void pack_rectangles(PackContext *context, PackRect *rects, u64 rects_size) {
    for (u32 i = 0; i < context->num_nodes-1; ++i) {
        context->nodes[i].next = &context->nodes[i+1];
    }
    context->nodes[context->num_nodes-1].next = NULL;

    context->active_list = &context->nodes[0];
    context->nodes[0].x = 0;
    context->nodes[0].y = 0;
    context->nodes[1].x = context->width;
    context->nodes[1].y = (1<<30);
    context->nodes[1].next = NULL;

    context->free_list = &context->nodes[2];
    
    quicksort(rects, sizeof(PackRect), rects_size, compare_pack_rects);
    for (u64 i = 0; i < rects_size; ++i) {
        if (rects[i].width > 0 && rects[i].height > 0) {
            u32 x, y;
            PackNode **best = find_best_pack_pos(context, rects[i].width, rects[i].height, &x, &y);

            // Check error
            if (!best || y + rects[i].height > context->height || context->free_list == NULL) {
                rects[i].x = -1;
                rects[i].y = -1;
                platform.log(LOG_ERROR, "failed to pack!");
                assert(false);
                return;
            }

            // Create new node
            PackNode *node = context->free_list;
            node->x = x;
            node->y = y + rects[i].height;
            context->free_list = node->next;

            PackNode *cur = *best;
            *best = node;
            // Insert new node

            // Remove nodes
            while (cur->next && cur->next->x <= x + rects[i].width) {
                PackNode *next = cur->next;
                cur->next = context->free_list;
                context->free_list = cur;
                cur = next;
            }

            node->next = cur;

            if (cur->x < x + rects[i].width) {
                cur->x = x + rects[i].width;
            }

            {
                PackNode *n = context->active_list;
                while (n->x < context->width) {
                    assert(n->x < n->next->x);
                    n = n->next;
                }
                assert(n->next == NULL);
            }

            {
                PackNode *n = NULL;
                u32 count = 0;

                n = context->active_list;
                while (n) {
                    n = n->next;
                    count++;
                }

                n = context->free_list;
                while (n) {
                    n = n->next;
                    count++;
                }
                assert(count == context->num_nodes);
            }

            rects[i].x = x;
            rects[i].y = y;
        } else {
            rects[i].x = 0;
            rects[i].y = 0;
        }
    }
}
