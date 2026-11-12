#include "../world/room_data.h"
#include "../assets.h"

const RoomDef room_darkruins_01 = {
    .name = "darkruins_01",
    .bg_img = &img_room,
    .floor = {FLOOR_X0, FLOOR_Y0, FLOOR_X1 - FLOOR_X0, FLOOR_Y1 - FLOOR_Y0},
    .colliders = {},
    .collider_count = 0,
    .entities = {},
    .entity_count = 0,
    .warps = {
        {FLOOR_X0, FLOOR_Y0 + 60, 20, 40, 9, START_X, START_Y, 0},
    },
    .warp_count = 1
};
