#ifndef BATTLE_H
#define BATTLE_H

#include <stdint.h>

void battle_start(void);
void battle_update(int held, int pressed);
void battle_draw(void);
int battle_finished(void);
int battle_won(void);

#endif
