#ifndef ROOM_DATA_H
#define ROOM_DATA_H

#include <stdint.h>

#define ROOM_MAX_ENTITIES 16
#define ROOM_MAX_WARPS 8
#define ROOM_MAX_COLLIDERS 32

typedef struct {
    int x, y, w, h;
} Rect;

typedef struct {
    int type; /* 0 = npc, 1 = interactable */
    int x, y;
    const char **dialogue;
    int dialogue_count;
    int after_action; /* 0 = none, 1 = battle */
    int sprite_id;
} Entity;

typedef struct {
    int x, y, w, h;
    int target_room;
    int target_x, target_y;
    int facing;
} Warp;

typedef struct {
    const char *name;
    const void *bg_img; /* pointer to Img */
    Rect floor;
    Rect colliders[ROOM_MAX_COLLIDERS];
    int collider_count;
    Entity entities[ROOM_MAX_ENTITIES];
    int entity_count;
    Warp warps[ROOM_MAX_WARPS];
    int warp_count;
} RoomDef;

#endif
