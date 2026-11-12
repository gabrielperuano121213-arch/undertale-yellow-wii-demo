#include "room.h"
#include "room_data.h"
#include "../game.h"
#include "../gfx.h"
#include <string.h>

static const RoomDef *current_room = NULL;
static int room_loaded = 0;

void room_init(void)
{
    current_room = NULL;
    room_loaded = 0;
}

void room_load(const RoomDef *room)
{
    if (!room) return;
    current_room = room;
    room_loaded = 1;
}

int room_blocked_point(int x, int y)
{
    if (!room_loaded || !current_room) return 1;
    
    /* Check floor bounds */
    if (x < current_room->floor.x || x > current_room->floor.x + current_room->floor.w ||
        y < current_room->floor.y || y > current_room->floor.y + current_room->floor.h)
        return 1;
    
    /* Check colliders */
    for (int i = 0; i < current_room->collider_count; i++) {
        Rect *r = (Rect*)&current_room->colliders[i];
        if (x >= r->x && x <= r->x + r->w && y >= r->y && y <= r->y + r->h)
            return 1;
    }
    
    return 0;
}

void room_update(int held, int pressed)
{
    /* Room update logic will be integrated with game state */
    (void)held;
    (void)pressed;
}

void room_draw(void)
{
    if (!room_loaded || !current_room) return;
    /* Background drawing handled by game logic for now */
}

void room_interact(void)
{
    /* Interaction logic handled by game logic for now */
}

const RoomDef *room_get_current(void)
{
    return current_room;
}
