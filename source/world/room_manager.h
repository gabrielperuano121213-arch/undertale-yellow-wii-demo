#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H

#include "room_data.h"

void room_manager_init(void);
void room_manager_register(int id, const RoomDef *room);
void room_manager_goto(int id, int x, int y);
int room_manager_get_current_id(void);
const RoomDef *room_manager_get(int id);

#endif
