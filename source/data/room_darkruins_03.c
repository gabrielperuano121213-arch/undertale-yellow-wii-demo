#include "../world/room_data.h"
#include "../assets.h"

const RoomDef room_darkruins_03 = {
    .name = "darkruins_03",
    .bg_img = &img_room,
    .floor = {FLOOR_X0, FLOOR_Y0, FLOOR_X1 - FLOOR_X0, FLOOR_Y1 - FLOOR_Y0},
    .colliders = {},
    .collider_count = 0,
    .entities = {},
    .entity_count = 0,
    .warps = {
        {FLOOR_X0, FLOOR_Y0 + 60, 20, 40, 11, START_X, START_Y, 0},
        {FLOOR_X1 - 30, FLOOR_Y0 + 60, 20, 40, 13, START_X, START_Y, 0},
    },
    .warp_count = 2
};
