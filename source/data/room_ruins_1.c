#include "../world/room_data.h"
#include "../assets.h"

static Rect ruins1_colliders[] = {
    /* Sign */
    {SIGN_X - 5, SIGN_Y - 6, 10, 6},
};

static Entity ruins1_entities[] = {
    /* Dummy */
    {0, DUMMY_X, DUMMY_Y, NULL, 0, 1, 21}, /* type 0 npc, after_action 1 battle, sprite_id 21 = dummy */
    /* Sign interaction handled separately for now */
};

const RoomDef room_ruins_1 = {
    .name = "ruins_1",
    .bg_img = &img_room,
    .floor = {FLOOR_X0, FLOOR_Y0, FLOOR_X1 - FLOOR_X0, FLOOR_Y1 - FLOOR_Y0},
    .colliders = {
        {SIGN_X - 5, SIGN_Y - 6, 10, 6},
    },
    .collider_count = 1,
    .entities = {
        {0, DUMMY_X, DUMMY_Y, NULL, 0, 1, 21},
    },
    .entity_count = 1,
    .warps = {},
    .warp_count = 0
};
