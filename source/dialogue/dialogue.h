#ifndef DIALOGUE_H
#define DIALOGUE_H

enum { DLG_AFTER_NONE, DLG_AFTER_BATTLE };

void dialogue_init(void);
void dialogue_set(int after, int n, const char *a, const char *b);
void dialogue_update(int pressed);
void dialogue_draw(int top);
int dialogue_active(void);
int dialogue_after(void);

#endif
