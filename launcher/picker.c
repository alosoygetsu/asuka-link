/* The language list: a flag and its name. On the first-run screen it is the button top right and the list drops under
   it; elsewhere it is at the foot of the menu on the left, and the list opens upwards. */
#include "launcher.h"

enum { W = 96, H = 14, SIDE_Y = SCREEN_H - 58 };

static bool s_open;
static int s_hover = PICKER_NONE;

static bool side(void) { return true; } /* at the menu's foot, on the first run too */
static int list_x(void) { return side() ? 8 : SCREEN_W - 104; }
static SDL_Rect button(void) { return side() ? (SDL_Rect){8, SIDE_Y, W, H} : (SDL_Rect){SCREEN_W - 96, 8, W - 8, H}; }
static int row_y(int row) { return (side() ? SIDE_Y - 2 - LANGS * H : 8 + H + 2) + row * H; }

void picker_draw(int lang, bool focused)
{
    const SDL_Rect b = button();
    fill(b.x, b.y, b.w, b.h, C_PANEL);
    outline(b.x, b.y, b.w, b.h, focused || s_open ? C_GOLD : C_EDGE);
    art_flag(b.x + 4, b.y + 2, lang);
    text_flat(b.x + 24, b.y + 3, C_TEXT, LANG_TABLE[lang]->name);
    for (int i = 0; i < 3; i++) { /* the arrow */
        fill(b.x + b.w - 10 + i, b.y + 5 + i, 7 - 2 * i, 1, C_TEXT);
    }
    if (!s_open) {
        return;
    }
    for (int i = 0; i < LANGS; i++) {
        fill(list_x(), row_y(i), W, H, i == lang ? C_SELECT : i == s_hover ? C_RULE : C_PANEL);
        art_flag(list_x() + 6, row_y(i) + 2, i);
        text_flat(list_x() + 28, row_y(i) + 3, i == lang ? C_ON_SELECT : C_TEXT, LANG_TABLE[i]->name);
    }
    outline(list_x(), row_y(0), W, LANGS * H, C_EDGE);
}

bool picker_is_open(void) { return s_open; }

void picker_set_open(bool open)
{
    s_open = open;
    s_hover = PICKER_NONE;
}

int picker_hit(int x, int y)
{
    const SDL_Rect b = button();
    if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) {
        return PICKER_BUTTON;
    }
    if (s_open && x >= list_x() && x < list_x() + W && y >= row_y(0) && y < row_y(LANGS)) {
        return (y - row_y(0)) / H;
    }
    return PICKER_NONE;
}

void picker_hover(int row) { s_hover = row; }
