#include "../world/room_data.h"
#include "../assets.h"

const RoomDef room_ruins_00 = {
    .name = "ruins_00",
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
    .warps = {
        {FLOOR_X1 - 30, FLOOR_Y0 + 60, 20, 40, 1, 70, 110, 0}, /* exit to ruins_01 */
    },
    .warp_count = 1
};
