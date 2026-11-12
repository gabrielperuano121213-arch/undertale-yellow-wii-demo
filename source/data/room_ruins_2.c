#include "../world/room_data.h"
#include "../assets.h"

const RoomDef room_ruins_2 = {
    .name = "ruins_2",
    .bg_img = &img_room,
    .floor = {FLOOR_X0, FLOOR_Y0, FLOOR_X1 - FLOOR_X0, FLOOR_Y1 - FLOOR_Y0},
    .colliders = {},
    .collider_count = 0,
    .entities = {},
    .entity_count = 0,
    .warps = {
        {FLOOR_X0 + 10, FLOOR_Y0 + 50, 20, 40, 0, START_X, START_Y, 0}, /* warp back to room 0 */
    },
    .warp_count = 1
};
