/* The home screen once the disc is read: Asuka City at dusk over the whole window, the menu down the left (side_draw,
   on the other pages too), the room top right (sala.c) and, bottom left, the game: the disc, the team and Play. */
#include "launcher.h"

#ifndef LAUNCHER_VERSION
#define LAUNCHER_VERSION "dev"
#endif

#define GAME_NAME "Digimon World 2003 (Europe)"

enum {
    MENU_Y = 54, MENU_H = 16, MENU_GAP = 2, QUIT_GAP = 10, NAME_X = 40,
    CHIPS_Y = 84, CHIP_H = 14, CHIP_GAP = 6, TITLE_Y = 100, SUB_Y = 126, CARD_Y = 142, CARD_H = 26, CARD_GAP = 8,
    INFO_Y = 172, BTN_Y = 186, BTN_H = 28, SCENE_BASE = 200
};

static bool inside(SDL_Rect r, int x, int y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

/* ---- where things are, for the language given: drawing, the mouse and --check use the same numbers ---- */

/* the menu, top to bottom; Quit a little apart */
static const short PAGES[5] = {S_PLAY, S_SETTINGS, S_SAVES, S_TOOLS, S_QUIT};

static SDL_Rect menu_rect(int i)
{
    return (SDL_Rect){0, MENU_Y + i * (MENU_H + MENU_GAP) + (i == 4 ? QUIT_GAP : 0), SIDE_W - 1, MENU_H};
}

static void chip_text(const Lang *l, int i, const char *tag, char *out, int size)
{
    if (i == 0) {
        SDL_snprintf(out, size, "%s SLES-03936", l->s[S_DISC_OK]);
    } else {
        SDL_snprintf(out, size, "%s %s", l->s[S_RELEASE], tag);
    }
}

static SDL_Rect chip_rect(const Lang *l, int i, const char *tag)
{
    char s[96];
    int x = MAIN_X;
    for (int k = 0; k < i; k++) {
        chip_text(l, k, tag, s, sizeof s);
        x += text_width(s, 1) + 12 + CHIP_GAP;
    }
    chip_text(l, i, tag, s, sizeof s);
    return (SDL_Rect){x, CHIPS_Y, text_width(s, 1) + 12, CHIP_H};
}

static SDL_Rect play_rect(const Lang *l) { return (SDL_Rect){MAIN_X, BTN_Y, SDL_max(120, text_width(l->s[S_PLAY], 2) + 52), BTN_H}; }

static int card_w(void) { return (MAIN_W - 2 * CARD_GAP) / 3; }

static void team_info(const Lang *l, const Team *t, char *out, int size)
{
    SDL_snprintf(out, size, "%s %s  -  %d:%02d", l->s[S_LAST_SAVE], t->when, t->hours, t->minutes);
}

int side_hit(int x, int y)
{
    for (int i = 0; i < 5; i++) {
        if (inside(menu_rect(i), x, y)) {
            return PAGES[i];
        }
    }
    return -1;
}

bool home_fits(const Lang *l)
{
    const Team worst = {.when = "2026-10-28 13:23", .hours = 999, .minutes = 59};
    char info[96];
    bool menu = true;
    for (int i = 0; i < 5; i++) {
        menu &= text_width(l->s[PAGES[i]], 1) <= SIDE_W - 20;
    }
    team_info(l, &worst, info, sizeof info);
    const SDL_Rect chip = chip_rect(l, 1, "v99.99.99");
    return menu && text_width(l->s[S_TAGLINE], 1) <= SIDE_W - 16 && text_width(l->s[S_READY], 2) <= MAIN_W &&
           chip.x + chip.w <= SCREEN_W - 8 && text_width(info, 1) <= MAIN_W && text_width(l->s[S_NO_TEAM], 1) <= 2 * MAIN_W - 60 &&
           text_width(l->s[S_SALA_ADD], 1) <= 130 && text_width(l->s[S_SALA], 1) + 30 <= 130;
}

int home_hit(int x, int y)
{
    const Row *r = rows();
    const char *tag = release_tag() ? release_tag() : "";
    const int page = side_hit(x, y), peer = sala_hit(x, y);
    for (int i = 0; i < list_rows(r); i++) {
        const bool hit = r[i].kind == R_PEER ? r[i].id == peer
                       : r[i].id == S_PLAY ? inside(play_rect(L), x, y)
                       : r[i].id == S_RELEASE ? inside(chip_rect(L, 1, tag), x, y)
                       : r[i].id == page;
        if (hit) {
            return i;
        }
    }
    return -1;
}

/* ---- the scene ---- */

static void disc(int cx, int cy, int r, Uint32 colour, Uint8 alpha)
{
    for (int dy = -r; dy <= r; dy++) {
        const int w = (int)SDL_sqrtf((float)(r * r - dy * dy));
        fill_alpha(cx - w, cy + dy, 2 * w + 1, 1, colour, alpha);
    }
}

/* Asuka City at dusk: a few stars, the sky going orange at the horizon, the sun setting behind two rows of towers with
   lit windows, and the link (the co-op) arcing between the antennas of the two tallest */
static Uint32 s_seed;

static int rnd(int n)
{
    s_seed = s_seed * 1103515245u + 12345u;
    return (int)((s_seed >> 16) % (Uint32)n);
}

static void towers(int near)
{
    for (int x = near ? -6 : 0; x < SCREEN_W;) {
        const int w = near ? 14 + rnd(18) : 10 + rnd(14), right = x * 100 / SCREEN_W; /* taller to the right */
        const int h = near ? 10 + rnd(16) + right * 2 / 10 : 18 + rnd(22) + right * 3 / 10, top = SCENE_BASE - h;
        fill(x, top, w, h, near ? C_CITY_NEAR : C_CITY_FAR);
        if (!near && rnd(3) == 0) {
            fill(x + w / 2, top - 4, 1, 4, C_CITY_FAR); /* a mast */
        }
        for (int wy = top + 3; wy < SCENE_BASE - 3; wy += 4) { /* its windows, some lit */
            for (int wx = x + 2; wx < x + w - 2; wx += 3) {
                if (rnd(near ? 9 : 6) == 0) {
                    fill_alpha(wx, wy, 1, 1, C_WINDOW, near ? 220 : 120);
                }
            }
        }
        x += w + (near ? rnd(4) : 1 + rnd(3));
    }
}

/* the tower at x, w wide, h tall, with an antenna; where its tip is */
static SDL_Point antenna(int x, int w, int h)
{
    const int top = SCENE_BASE - h;
    fill(x, top, w, h, C_CITY_NEAR);
    fill(x + 1, top - 3, w - 2, 3, C_CITY_NEAR);
    fill(x + w / 2, top - 16, 1, 13, C_CITY_NEAR);
    fill_alpha(x + w / 2 - 1, top - 17, 3, 3, C_LINK, 110);
    fill(x + w / 2, top - 16, 1, 1, C_LINK);
    for (int wy = top + 4; wy < SCENE_BASE - 3; wy += 5) {
        fill_alpha(x + 2, wy, w - 4, 1, C_WINDOW, (Uint8)(wy % 3 ? 60 : 150));
    }
    return (SDL_Point){x + w / 2, top - 16};
}

static void scene(void)
{
    s_seed = 2003;
    for (int i = 0; i < 30; i++) {
        fill(150 + rnd(SCREEN_W - 150), 6 + rnd(84), 1, 1, i % 4 ? C_FAINT : C_SOFT);
    }
    for (int y = 90; y < SCENE_BASE; y++) { /* the dusk: deep blue, violet, then the horizon's rose */
        const int t = (y - 90) * 256 / (SCENE_BASE - 90);
        fill_alpha(0, y, SCREEN_W, 1, t < 110 ? C_DUSK_HI : t < 200 ? C_DUSK : C_DUSK_LOW, (Uint8)(40 + t * 150 / 256));
    }
    const int sx = 368, sy = SCENE_BASE - 50;
    disc(sx, sy, 60, C_DUSK_LOW, 30);
    disc(sx, sy, 44, C_SUN, 34);
    disc(sx, sy, 30, C_SUN, 255);
    disc(sx - 4, sy - 5, 19, C_SUN_HI, 255);
    for (int y = sy + 3, gap = 1; y < sy + 31; y += 3 + gap / 2, gap++) { /* the stripes across its lower half */
        fill_alpha(sx - 31, y, 63, 1 + gap / 3, C_DUSK, 255);
    }
    towers(0);
    const SDL_Point a = antenna(286, 13, 74), b = antenna(446, 15, 92);
    towers(1);
    for (int i = 0; i <= 64; i++) { /* the link: an arc of dots from one antenna to the other */
        const float t = i / 64.0f, x = a.x + (b.x - a.x) * t, y = a.y + (b.y - a.y) * t - 34 * 4 * t * (1 - t);
        fill_alpha((int)x - 1, (int)y - 1, 3, 3, C_LINK, 40);
        if (i % 2 == 0) {
            fill((int)x, (int)y, 1, 1, C_LINK);
        }
        if (i == 21 || i == 43) { /* two packets on their way */
            fill((int)x - 1, (int)y - 1, 3, 3, C_TEXT);
        }
    }
}

/* ---- the screen ---- */

void side_draw(int page, int cur)
{
    const Lang *l = L;
    fill_alpha(0, 0, SIDE_W, SCREEN_H, C_BG, 225);
    fill(SIDE_W - 1, 0, 1, SCREEN_H, C_RULE);
    art_icon(8, 8);
    text(NAME_X, 15, 1, C_GOLD, APP_NAME);
    text(8, 33, 1, C_SOFT, l->s[S_TAGLINE]);
    fill(8, 46, SIDE_W - 16, 1, C_RULE);
    for (int i = 0; i < 5; i++) {
        const SDL_Rect t = menu_rect(i);
        const bool sel = PAGES[i] == cur, here = PAGES[i] == page;
        if (i == 4) {
            fill(8, t.y - QUIT_GAP / 2 - 1, SIDE_W - 16, 1, C_RULE);
        }
        if (sel) {
            fill(t.x + 6, t.y, t.w - 12, t.h, C_SELECT);
        } else if (here) {
            outline(t.x + 6, t.y, t.w - 12, t.h, C_LINK);
            outline(t.x + 7, t.y + 1, t.w - 14, t.h - 2, C_LINK);
        }
        text_sel(14, t.y + 4, sel, here ? C_LINK : C_SOFT, l->s[PAGES[i]]);
    }
    text(8, SCREEN_H - 36, 1, C_FAINT, LAUNCHER_VERSION);
    text(8, SCREEN_H - 24, 1, C_FAINT, "by alosoygetsu");
}

static void draw_chips(const Lang *l, int cur)
{
    const char *tag = release_tag();
    for (int i = 0; i < (tag ? 2 : 1); i++) {
        char s[96];
        const SDL_Rect c = chip_rect(l, i, tag ? tag : "");
        const bool news = i == 1, sel = news && cur == S_RELEASE;
        chip_text(l, i, tag ? tag : "", s, sizeof s);
        fill_alpha(c.x, c.y, c.w, c.h, sel ? C_SELECT : C_PANEL, 225);
        outline(c.x, c.y, c.w, c.h, news ? C_GOLD : C_EDGE);
        text_sel(c.x + 6, c.y + 3, sel, news ? C_GOLD : C_TEXT, s);
    }
}

static void draw_play(const Lang *l, bool play)
{
    const SDL_Rect p = play_rect(l);
    fill_alpha(p.x, p.y + 3, p.w, p.h, 0x000000, 120);
    fill(p.x, p.y, p.w, p.h, play ? C_GOLD_HI : C_GOLD);
    fill_alpha(p.x, p.y + p.h - 3, p.w, 3, 0x000000, 50);
    if (play) {
        outline(p.x, p.y, p.w, p.h, C_TEXT);
    }
    triangle(p.x + 14, p.y + BTN_H / 2, 6, 1, C_BG);
    text(p.x + 34, p.y + (BTN_H - 2 * LINE_H) / 2 + 1, 2, C_BG, l->s[S_PLAY]);
}

/* the party, as cards: a coloured square with the partner's initial, the name and the level; the last save under them */
static void draw_team(const Lang *l)
{
    const Team *t = team_get();
    char info[96];
    if (!t) {
        wrap(MAIN_X, CARD_Y + 2, MAIN_W, C_SOFT, l->s[S_NO_TEAM]);
        return;
    }
    for (int i = 0; i < t->count; i++) {
        const int x = MAIN_X + i * (card_w() + CARD_GAP);
        char level[32], initial[2] = {team_name(t->partner[i])[0], 0};
        fill_alpha(x, CARD_Y, card_w(), CARD_H, C_PANEL, 225);
        outline(x, CARD_Y, card_w(), CARD_H, C_EDGE);
        fill(x + 3, CARD_Y + 3, 20, 20, team_colour(t->partner[i]));
        text(x + 3 + (20 - text_width(initial, 1)) / 2, CARD_Y + 7, 1, C_BG, initial);
        SDL_snprintf(level, sizeof level, "%s %d", l->s[S_LEVEL], t->level[i]);
        text(x + 28, CARD_Y + 1, 1, C_TEXT, team_name(t->partner[i]));
        text(x + 28, CARD_Y + 13, 1, C_SOFT, level);
    }
    team_info(l, t, info, sizeof info);
    text(MAIN_X, INFO_Y, 1, C_SOFT, info);
}

void home_draw(void)
{
    const Lang *l = L;
    const Row *r = &rows()[g_app.sel[SC_HOME]];
    const int cur = r->kind == R_ACTION ? r->id : -1;
    scene();
    side_draw(S_PLAY, cur == S_PLAY || cur == S_RELEASE ? -1 : cur);
    sala_draw(r->kind == R_PEER ? r->id : -1);
    draw_chips(l, cur);
    text(MAIN_X, TITLE_Y, 2, C_TEXT, l->s[S_READY]);
    text(MAIN_X, SUB_Y, 1, C_SOFT, GAME_NAME);
    draw_team(l);
    draw_play(l, cur == S_PLAY);
}
