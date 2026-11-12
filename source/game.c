/* Demake de Undertale Yellow para Wii: titulo -> sala de las ruinas -> dialogos -> menu (C) ->
 * batalla con el ALMA AMARILLA (se mueve con la cruceta y DISPARA con Z). Todo a 30 ticks/s. */
#include <stdio.h>
#include <string.h>
#include "game.h"
#include "gfx.h"

enum { S_TITLE, S_ROOM, S_DIALOG, S_MENU, S_BATTLE, S_GAMEOVER };
enum { B_INTRO, B_MENU, B_FIGHT, B_ATTACK, B_STRIKE, B_ACT, B_ITEM, B_MERCY, B_TEXT, B_SHRINK, B_DODGE, B_GROW };
enum { AFTER_NONE, AFTER_BATTLE };               /* que pasa al acabar un dialogo */
enum { NEXT_TURN, NEXT_WIN, NEXT_FLEE };         /* que pasa al acabar un texto de batalla */

#define E_HPMAX 30
#define BOX_X 19                                 /* caja de batalla normal (interior) */
#define BOX_Y 128
#define BOX_W 282
#define BOX_H 64
#define DODGE_X 128                              /* caja pequena del turno enemigo */
#define DODGE_W 64

static const Img *const BTN[4] = { img_btn_fight, img_btn_act, img_btn_item, img_btn_mercy };
static const int BTN_X[4] = { 16, 92, 172, 250 };
#define BTN_Y 216

static int state, tick;
static uint32_t held, pressed, prev_held;
static unsigned rng_s = 0x2545F491u;

static int rnd(int n) { rng_s = rng_s * 1664525u + 1013904223u; return (int)((rng_s >> 12) % (unsigned)n); }
static int iabs(int v) { return v < 0 ? -v : v; }
static int hit(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

/* ---------------------------------------------------------------- estado del mundo */
static int px, py, face, walk_t;                 /* pies de Clover; face: 0 abajo,1 arriba,2 izq,3 der */
static int hp, hpmax = 20, snacks;
static int dummy_state;                          /* 0 nuevo, 1 perdonado, 2 derrotado */
static int menu_sel, menu_view;
static char tb[128];                             /* temporal para dibujar */
static char msg[96];                             /* mensaje persistente (texto de batalla) */

/* ---------------------------------------------------------------- texto con efecto maquina de escribir */
static const char *ty_s = "";
static int ty_n, ty_len, ty_wait;

static void ty_set(const char *s) { ty_s = s; ty_n = 0; ty_len = (int)strlen(s); ty_wait = 0; }
static int  ty_done(void) { return ty_n >= ty_len; }
static void ty_finish(void) { ty_n = ty_len; }
static void ty_step(void)
{
    if (ty_n >= ty_len) return;
    if (ty_wait > 0) { ty_wait--; return; }
    char c = ty_s[ty_n++];
    if (c == ',' || c == '.' || c == '!' || c == '?') ty_wait = 3;
}

static void box(int x, int y, int w, int h)
{
    gfx_rect(x, y, w, h, C_WHITE);
    gfx_rect(x + 3, y + 3, w - 6, h - 6, C_BLACK);
}

static void text_center(const Font *f, int y, const char *s, uint8_t c, int scale)
{
    gfx_text(f, (SCR_W - gfx_text_w(f, s, scale)) / 2, y, s, c, scale);
}

/* ---------------------------------------------------------------- dialogos del mundo */
static const char *dlg[3];
static int dlg_n, dlg_i, dlg_after, dlg_top;

static void say(int after, int n, const char *a, const char *b)
{
    dlg[0] = a; dlg[1] = b; dlg_n = n; dlg_i = 0; dlg_after = after;
    dlg_top = py > 130;
    ty_set(dlg[0]);
    state = S_DIALOG;
}

/* ---------------------------------------------------------------- batalla */
static int bst, bsel, bsub, btimer;
static int e_hp, e_gone, spareable, killed, turn_no, next_after;
static int bx, by, bw, bh;                       /* interior de la caja */
static int sx, sy, inv, cd;                      /* alma */
static int bar_x, dmg, shake, dodge_t, go_timer;
typedef struct { int x, y, vx, vy, on; } Obj;    /* 1/16 de pixel */
static Obj pel[24], shot[6];

static const char *const FLAVOR[3] = {
    "* The dummy stands there,\n  doing dummy things.",
    "* Smells like cotton and\n  determination.",
    "* The dummy is... staring?"
};

static void start_battle(void)
{
    state = S_BATTLE; bst = B_INTRO; btimer = 0; e_hp = E_HPMAX; e_gone = 0; spareable = 0; killed = 0;
    turn_no = 0; bsel = 0; bx = BOX_X; by = BOX_Y; bw = BOX_W; bh = BOX_H; shake = 0; inv = 0; cd = 0;
    memset(pel, 0, sizeof pel); memset(shot, 0, sizeof shot);
}

static void end_battle(int win)
{
    state = S_ROOM;
    if (win) dummy_state = killed ? 2 : 1;
}

static void btext(const char *s, int next) { ty_set(s); next_after = next; bst = B_TEXT; }

static void strike(int d)
{
    dmg = d; bst = B_STRIKE; btimer = 0;
    e_hp -= d; if (e_hp < 0) e_hp = 0;
    shake = d > 0 ? 20 : 0;
}

static void dodge_update(void)
{
    int sp = (held & IN_X) ? 1 : 2;              /* mantener X = mas lento, como en Undertale */
    int dx = !!(held & IN_RIGHT) - !!(held & IN_LEFT), dy = !!(held & IN_DOWN) - !!(held & IN_UP);
    sx += dx * sp; sy += dy * sp;
    if (sx < bx + 5) sx = bx + 5;
    if (sx > bx + bw - 5) sx = bx + bw - 5;
    if (sy < by + 5) sy = by + 5;
    if (sy > by + bh - 5) sy = by + bh - 5;
    if (inv > 0) inv--;
    if (cd > 0) cd--;

    if ((pressed & IN_Z) && cd == 0) {           /* el alma amarilla dispara hacia arriba */
        for (int i = 0; i < 6; i++)
            if (!shot[i].on) { shot[i].x = sx * 16; shot[i].y = (sy - 6) * 16; shot[i].vx = 0; shot[i].vy = -56; shot[i].on = 1; cd = 7; break; }
    }
    if (dodge_t > 14 && dodge_t % 9 == 0) {      /* el dummy lanza pelotas de relleno */
        for (int i = 0; i < 24; i++)
            if (!pel[i].on) {
                pel[i].x = (bx + 6 + rnd(bw - 12)) * 16; pel[i].y = (by + 1) * 16;
                pel[i].vx = (rnd(7) - 3) * 2; pel[i].vy = 24 + rnd(16); pel[i].on = 1; break;
            }
    }
    for (int i = 0; i < 6; i++) if (shot[i].on) {
        shot[i].y += shot[i].vy;
        if (shot[i].y < (by - 6) * 16) shot[i].on = 0;
    }
    for (int i = 0; i < 24; i++) if (pel[i].on) {
        pel[i].x += pel[i].vx; pel[i].y += pel[i].vy;
        int x = pel[i].x / 16, y = pel[i].y / 16;
        if (y > by + bh + 4 || x < bx - 4 || x > bx + bw + 4) { pel[i].on = 0; continue; }
        for (int j = 0; j < 6; j++)               /* un disparo destruye la pelota */
            if (shot[j].on) {
                int ddx = shot[j].x / 16 - x, ddy = shot[j].y / 16 - y;
                if (ddx * ddx + ddy * ddy < 36) { shot[j].on = 0; pel[i].on = 0; break; }
            }
        if (pel[i].on && inv == 0 && iabs(x - sx) < 5 && iabs(y - sy) < 5) {
            pel[i].on = 0; hp -= 3; inv = 30;
            if (hp <= 0) { hp = 0; state = S_GAMEOVER; go_timer = 0; return; }
        }
    }
    if (--dodge_t <= 0) {
        memset(pel, 0, sizeof pel); memset(shot, 0, sizeof shot);
        bst = B_GROW; btimer = 0;
    }
}

static void battle_update(void)
{
    switch (bst) {
    case B_INTRO:
        if (++btimer >= 36) { bst = B_MENU; ty_set("* TRAINING DUMMY blocks the\n  way!"); }
        break;
    case B_MENU:
        ty_step();
        if (pressed & IN_LEFT)  bsel = (bsel + 3) & 3;
        if (pressed & IN_RIGHT) bsel = (bsel + 1) & 3;
        if (pressed & IN_Z) {
            bsub = 0;
            if (bsel == 0) bst = B_FIGHT;
            else if (bsel == 1) bst = B_ACT;
            else if (bsel == 2) { if (snacks > 0) bst = B_ITEM; }
            else bst = B_MERCY;
        }
        break;
    case B_FIGHT:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_Z) { bst = B_ATTACK; bar_x = 22; }
        break;
    case B_ATTACK:
        bar_x += 6;
        if (pressed & IN_Z) { int d = iabs(bar_x - 160); strike(d > 140 ? 0 : 1 + (140 - d) * 19 / 140); }
        else if (bar_x > 298) strike(0);
        break;
    case B_STRIKE:
        if (shake > 0) shake--;
        if (++btimer >= 32) {
            if (e_hp <= 0) { killed = 1; e_gone = 1; btext("* YOU WON!\n* You earned 0 EXP and 0 gold.", NEXT_WIN); }
            else { bst = B_SHRINK; btimer = 0; }
        }
        break;
    case B_ACT:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_UP) bsub = 0;
        else if (pressed & IN_DOWN) bsub = 1;
        else if (pressed & IN_Z) {
            if (bsub == 0) btext("* TRAINING DUMMY - ATK 0 DEF 0\n* A fake foe for practice.\n* Dodge its pellets with care.", NEXT_TURN);
            else { spareable = 1; btext("* You cheer for the dummy.\n* It seems... content.", NEXT_TURN); }
        }
        break;
    case B_ITEM:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_Z) {
            int before = hp;
            snacks--; hp += 10; if (hp > hpmax) hp = hpmax;
            if (hp == before) snprintf(msg, sizeof msg, "* You ate the SNACK.\n* Your HP was maxed out.");
            else snprintf(msg, sizeof msg, "* You ate the SNACK.\n* You recovered %d HP.", hp - before);
            btext(msg, NEXT_TURN);
        }
        break;
    case B_MERCY:
        if (pressed & IN_X) bst = B_MENU;
        else if (pressed & IN_UP) bsub = 0;
        else if (pressed & IN_DOWN) bsub = 1;
        else if (pressed & IN_Z) {
            if (bsub == 1) btext("* Got away safely...", NEXT_FLEE);
            else if (spareable) { killed = 0; e_gone = 1; btext("* YOU WON!\n* You earned 0 EXP and 0 gold.", NEXT_WIN); }
            else btext("* You offer mercy, but the\n  dummy isn't ready.", NEXT_TURN);
        }
        break;
    case B_TEXT:
        ty_step();
        if (held & IN_X) ty_step();              /* mantener X acelera el texto */
        if ((pressed & IN_Z) && ty_done()) {
            if (next_after == NEXT_TURN) { bst = B_SHRINK; btimer = 0; }
            else end_battle(next_after == NEXT_WIN);
        }
        break;
    case B_SHRINK: {
        int t = ++btimer;
        bx = BOX_X + (DODGE_X - BOX_X) * t / 6; bw = BOX_W + (DODGE_W - BOX_W) * t / 6;
        if (t >= 6) { bst = B_DODGE; dodge_t = 150; sx = bx + bw / 2; sy = by + bh / 2; inv = 0; cd = 0; }
        break;
    }
    case B_DODGE:
        dodge_update();
        break;
    case B_GROW: {
        int t = ++btimer;
        bx = DODGE_X + (BOX_X - DODGE_X) * t / 6; bw = DODGE_W + (BOX_W - DODGE_W) * t / 6;
        if (t >= 6) { bst = B_MENU; turn_no++; ty_set(FLAVOR[turn_no % 3]); }
        break;
    }
    }
}

/* ---------------------------------------------------------------- dibujo de la batalla */
static void draw_hp_row(void)
{
    gfx_text(&font_stats, 16, 199, "CLOVER", C_WHITE, 2);
    gfx_text(&font_stats, 96, 199, "LV 1", C_WHITE, 2);
    gfx_text(&font_stats, 150, 203, "HP", C_WHITE, 1);
    gfx_rect(168, 198, hpmax, 11, C_RED);
    if (hp > 0) gfx_rect(168, 198, hp, 11, C_YELLOW);
    snprintf(tb, sizeof tb, "%d / %d", hp, hpmax);
    gfx_text(&font_stats, 168 + hpmax + 8, 199, tb, C_WHITE, 2);
}

static void draw_buttons(int active)
{
    for (int i = 0; i < 4; i++)
        gfx_img(&BTN[i][(active && i == bsel) ? 1 : 0], BTN_X[i], BTN_Y);
}

static void list_row(int i, const char *s, uint8_t c)
{
    gfx_text(&font_main, bx + 28, by + 8 + 18 * i, s, c, 1);
    if (i == bsub) gfx_sprite(&img_soul[0], bx + 17, by + 8 + 18 * i + 8);
}

static void draw_battle(void)
{
    gfx_clear(C_BLACK);
    if (bst == B_INTRO) {                        /* destello del alma y vuelo al boton FIGHT */
        int t = btimer, x = px, y = py - 10;
        if (t < 18) { if ((t / 3) % 2 == 0) gfx_sprite(&img_soul[0], x, y); }
        else {
            int k = t - 18, tx = BTN_X[0] + 8, ty = BTN_Y + 11;
            gfx_sprite(&img_soul[0], x + (tx - x) * k / 18, y + (ty - y) * k / 18);
        }
        return;
    }
    if (!e_gone) gfx_sprite(&img_dummy, 160 + (shake > 0 ? ((shake & 1) ? 4 : -4) : 0), 112);

    gfx_rect(bx - 3, by - 3, bw + 6, bh + 6, C_WHITE);
    gfx_rect(bx, by, bw, bh, C_BLACK);

    switch (bst) {
    case B_MENU: case B_TEXT:
        gfx_text_n(&font_main, bx + 12, by + 10, ty_s, ty_n, C_WHITE, 1);
        break;
    case B_FIGHT: list_row(0, "* TRAINING DUMMY", spareable ? C_YELLOW : C_WHITE); break;
    case B_ACT:   list_row(0, "* Check", C_WHITE); list_row(1, "* Cheer", C_WHITE); break;
    case B_ITEM:  list_row(0, "* Snack", C_WHITE); break;
    case B_MERCY: list_row(0, "* Spare", spareable ? C_YELLOW : C_WHITE); list_row(1, "* Flee", C_WHITE); break;
    case B_ATTACK: case B_STRIKE:
        gfx_rect(bx + 6, by + 8, bw - 12, bh - 16, C_WHITE);
        gfx_rect(bx + 9, by + 11, bw - 18, bh - 22, C_BLACK);
        gfx_rect(159, by + 11, 2, bh - 22, C_YELLOW);
        gfx_rect(bar_x - 3, by + 5, 7, bh - 10, C_BLACK);
        gfx_rect(bar_x - 2, by + 6, 5, bh - 12, (bst == B_STRIKE && (btimer / 2) % 2) ? C_YELLOW : C_WHITE);
        break;
    case B_DODGE:
        for (int i = 0; i < 6; i++) if (shot[i].on) gfx_sprite(&img_shot[3], shot[i].x / 16, shot[i].y / 16);
        for (int i = 0; i < 24; i++) if (pel[i].on) {
            int x = pel[i].x / 16, y = pel[i].y / 16;
            gfx_rect(x - 1, y - 3, 3, 7, C_WHITE); gfx_rect(x - 3, y - 1, 7, 3, C_WHITE); gfx_rect(x - 2, y - 2, 5, 5, C_WHITE);
        }
        gfx_sprite(&img_soul[(inv > 0 && (inv / 2) % 2) ? 1 : 0], sx, sy);
        break;
    default: break;
    }
    if (bst == B_STRIKE && btimer > 6) {         /* numero de dano y barra de vida del enemigo */
        const char *s = tb;
        if (dmg > 0) snprintf(tb, sizeof tb, "%d", dmg); else snprintf(tb, sizeof tb, "MISS");
        gfx_text(&font_stats, (SCR_W - gfx_text_w(&font_stats, s, 3)) / 2, 36, s, dmg > 0 ? C_RED : C_LGRAY, 3);
        gfx_rect(130, 62, 60, 8, C_GRAY);
        gfx_rect(130, 62, 60 * e_hp / E_HPMAX, 8, C_GREEN);
    }
    draw_hp_row();
    draw_buttons(bst == B_MENU || bst == B_FIGHT || bst == B_ACT || bst == B_ITEM || bst == B_MERCY || bst == B_TEXT);
    if (bst == B_MENU || bst == B_TEXT) {        /* el alma sobre el boton elegido */
        if (bst == B_MENU) gfx_sprite(&img_soul[0], BTN_X[bsel] + 8, BTN_Y + 11);
    }
}

/* ---------------------------------------------------------------- mundo: sala de las ruinas */
static int blocked(int x, int y)                 /* x,y = pies */
{
    if (x < FLOOR_X0 + 6 || x > FLOOR_X1 - 6 || y < FLOOR_Y0 + 8 || y > FLOOR_Y1 - 2) return 1;
    if (hit(x - 6, y - 4, 12, 5, SIGN_X - 5, SIGN_Y - 6, 10, 6)) return 1;
    if (dummy_state != 2 && hit(x - 6, y - 4, 12, 5, DUMMY_X - 11, DUMMY_Y - 5, 22, 7)) return 1;
    return 0;
}

static void interact(void)
{
    int ox = px - 6 + (face == 3 ? 10 : face == 2 ? -10 : 0);        /* zona "frente a Clover" */
    int oy = py - 12 + (face == 0 ? 10 : face == 1 ? -10 : 0);
    if (dummy_state != 2 && hit(ox, oy, 12, 12, DUMMY_X - 14, DUMMY_Y - 44, 28, 50)) {
        if (dummy_state == 0) say(AFTER_BATTLE, 1, "* ... A TRAINING DUMMY.\n* It seems ready to be shot.", NULL);
        else say(AFTER_NONE, 1, "* The dummy stands peacefully.\n* It looks happy.", NULL);
    } else if (hit(ox, oy, 12, 12, SIGN_X - 10, SIGN_Y - 18, 20, 22)) {
        say(AFTER_NONE, 2, "* CONTROLS:\n* Z = Wiimote 2 (OK / shoot)\n* X = Wiimote 1 (cancel / slow)",
            "* C = Wiimote B (menu)\n* D-pad moves. HOME exits.\n* Shoot the dummy... or don't.");
    } else if (hit(ox, oy, 12, 12, FLOWERS_X - 8, FLOWERS_Y - 8, FLOWERS_W + 16, FLOWERS_H + 16)) {
        say(AFTER_NONE, 1, "* A bed of golden flowers.\n* They look warm and soft.", NULL);
    }
}

static void room_update(void)
{
    int dx = !!(held & IN_RIGHT) - !!(held & IN_LEFT), dy = !!(held & IN_DOWN) - !!(held & IN_UP);
    if (pressed & IN_C) { state = S_MENU; menu_sel = 0; menu_view = -1; return; }
    if (pressed & IN_Z) { interact(); if (state != S_ROOM) return; }
    if (dx || dy) {
        if (dy > 0) face = 0; else if (dy < 0) face = 1;
        if (dx < 0) face = 2; else if (dx > 0) face = 3;
        for (int i = 0; i < 3; i++) {            /* 3 px por tick, de 1 en 1 para deslizar por las paredes */
            if (dx && !blocked(px + dx, py)) px += dx;
            if (dy && !blocked(px, py + dy)) py += dy;
        }
        walk_t++;
    } else walk_t = 0;
}

static void dialog_update(void)
{
    ty_step();
    if (pressed & IN_X) ty_finish();
    if (pressed & IN_Z) {
        if (!ty_done()) ty_finish();
        else if (++dlg_i < dlg_n) ty_set(dlg[dlg_i]);
        else if (dlg_after == AFTER_BATTLE) start_battle();
        else state = S_ROOM;
    }
}

static void menu_update(void)
{
    if (menu_view >= 0) { if (pressed & (IN_Z | IN_X | IN_C)) menu_view = -1; return; }
    if (pressed & (IN_X | IN_C)) { state = S_ROOM; return; }
    if (pressed & IN_UP) menu_sel = (menu_sel + 2) % 3;
    if (pressed & IN_DOWN) menu_sel = (menu_sel + 1) % 3;
    if (pressed & IN_Z) menu_view = menu_sel;
}

static void gameover_update(void)
{
    go_timer++;
    if (go_timer > 60 && (pressed & IN_Z)) {
        hp = hpmax; snacks = 2; dummy_state = 0; px = START_X; py = START_Y; face = 0; state = S_ROOM;
    }
}

/* ---------------------------------------------------------------- dibujo del mundo */
static void draw_clover(void)
{
    const Img *set = face == 0 ? img_clover_down : face == 1 ? img_clover_up : face == 2 ? img_clover_left : img_clover_right;
    int n = face < 2 ? 4 : 2;
    const Img *im = &set[walk_t ? (walk_t / 4) % n : 0];
    gfx_img(im, px - im->ox, py - im->h + 1);
}

static void draw_room(void)
{
    gfx_img(&img_room, 0, 0);
    int dummy_vis = dummy_state != 2, dummy_front = DUMMY_Y + 1 > py;
    if (dummy_vis && !dummy_front) gfx_sprite(&img_dummy, DUMMY_X, DUMMY_Y);
    draw_clover();
    if (dummy_vis && dummy_front) gfx_sprite(&img_dummy, DUMMY_X, DUMMY_Y);
}

static void draw_dialog(void)
{
    int y = dlg_top ? 10 : 164;
    box(16, y, 288, 62);
    gfx_text_n(&font_main, 28, y + 8, ty_s, ty_n, C_WHITE, 1);
}

static void draw_menu(void)
{
    box(16, 14, 88, 82);
    gfx_text(&font_main, 26, 20, "CLOVER", C_WHITE, 1);
    gfx_text(&font_main, 26, 36, "LV  1", C_WHITE, 1);
    snprintf(tb, sizeof tb, "HP %d/%d", hp, hpmax);
    gfx_text(&font_main, 26, 52, tb, C_WHITE, 1);
    gfx_text(&font_main, 26, 68, "G  0", C_WHITE, 1);
    box(16, 100, 88, 70);
    gfx_text(&font_main, 40, 108, "ITEM", C_WHITE, 1);
    gfx_text(&font_main, 40, 126, "STAT", C_WHITE, 1);
    gfx_text(&font_main, 40, 144, "CELL", C_WHITE, 1);
    gfx_sprite(&img_soul[0], 31, 108 + 8 + 18 * menu_sel);
    if (menu_view >= 0) {
        box(112, 14, 192, 106);
        if (menu_view == 0) {
            if (snacks > 0) { snprintf(tb, sizeof tb, "SNACK x%d\n(Heals 10 HP)", snacks); gfx_text(&font_main, 124, 22, tb, C_WHITE, 1); }
            else gfx_text(&font_main, 124, 22, "(No items)", C_WHITE, 1);
        } else if (menu_view == 1) {
            gfx_text(&font_main, 124, 22, "\"CLOVER\"\nLV 1  HP 20/20\nAT 10  DF 10\nEXP 0  NEXT 10", C_WHITE, 1);
        } else gfx_text(&font_main, 124, 22, "No signal.\n(Maybe in the next\n demo...)", C_WHITE, 1);
    }
}

static void draw_title(void)
{
    gfx_img(&img_splash, 0, 0);
    if ((tick / 15) % 2 == 0) text_center(&font_stats, 186, "PRESS 2 TO START", C_WHITE, 2);
    text_center(&font_stats, 220, "FAN DEMAKE OF UNDERTALE YELLOW", C_GRAY, 1);
    text_center(&font_stats, 229, "ORIGINAL GAME BY TEAM UNDERTALE YELLOW", C_GRAY, 1);
}

static void draw_gameover(void)
{
    gfx_clear(C_BLACK);
    text_center(&font_main, 56, "GAME OVER", C_WHITE, 3);
    text_center(&font_main, 130, "Stay determined...", C_WHITE, 1);
    if (go_timer > 60 && (tick / 15) % 2 == 0) text_center(&font_stats, 190, "PRESS 2 TO TRY AGAIN", C_GRAY, 2);
}

static void render(void)
{
    switch (state) {
    case S_TITLE: draw_title(); break;
    case S_ROOM: draw_room(); break;
    case S_DIALOG: draw_room(); draw_dialog(); break;
    case S_MENU: draw_room(); draw_menu(); break;
    case S_BATTLE: draw_battle(); break;
    default: draw_gameover(); break;
    }
}

/* ---------------------------------------------------------------- API */
void game_init(void)
{
    state = S_TITLE; tick = 0; prev_held = 0;
    hp = hpmax; snacks = 2; dummy_state = 0;
    px = START_X; py = START_Y; face = 0; walk_t = 0;
    render();
}

void game_tick(uint32_t h)
{
    held = h; pressed = h & ~prev_held; prev_held = h; tick++;
    switch (state) {
    case S_TITLE:
        if (pressed & IN_Z) say(AFTER_NONE, 1, "* A quiet purple hall.\n* Golden flowers, a sign...\n* and something standing guard.", NULL);
        break;
    case S_ROOM: room_update(); break;
    case S_DIALOG: dialog_update(); break;
    case S_MENU: menu_update(); break;
    case S_BATTLE: battle_update(); break;
    default: gameover_update(); break;
    }
    render();
}
