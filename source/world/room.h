#ifndef ROOM_H
#define ROOM_H

#include <stdint.h>
#include "room_data.h"

void room_init(void);
void room_load(const RoomDef *room);
void room_update(int held, int pressed);
void room_draw(void);
void room_interact(void);
int room_blocked_point(int x, int y);
const RoomDef *room_get_current(void);

#endif
