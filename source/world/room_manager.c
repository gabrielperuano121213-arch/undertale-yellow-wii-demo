#include "room_manager.h"
#include "room.h"
#include "../data/rooms.h"
#include "../game.h"
#include <string.h>

static const RoomDef *rooms[32];
static int room_count = 0;
static int current_room_id = 0;

void room_manager_init(void)
{
    room_count = 0;
    current_room_id = 0;
    memset(rooms, 0, sizeof(rooms));
}

void room_manager_register(int id, const RoomDef *room)
{
    if (id >= 0 && id < 32) {
        rooms[id] = room;
        if (id >= room_count) room_count = id + 1;
    }
}

void room_manager_goto(int id, int x, int y)
{
    if (id >= 0 && id < 32 && rooms[id]) {
        current_room_id = id;
        room_load(rooms[id]);
        /* Player position will be set by game logic */
        (void)x; (void)y;
    }
}

int room_manager_get_current_id(void)
{
    return current_room_id;
}

const RoomDef *room_manager_get(int id)
{
    if (id >= 0 && id < 32) return rooms[id];
    return NULL;
}
