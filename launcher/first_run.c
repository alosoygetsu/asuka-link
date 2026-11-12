/* The screen that asks for the disc, right of the steps (home.c side_steps), over the city: a welcome, what's needed,
   and a panel to drop the disc on, with a disc drawn that spins while it's read. The reading goes on a thread
   (extract.c), so that the window answers and it can be cancelled or closed. */
#include "launcher.h"
#include "extract.h"

enum {
    ZONE_X = MAIN_X, ZONE_Y = 90, ZONE_W = MAIN_W, ZONE_H = 104, BUTTON_W = 128, BUTTON_H = 18,
    BUTTON_X = MAIN_X + (MAIN_W - BUTTON_W) / 2, BUTTON_Y = ZONE_Y + 76, DISC_X = MAIN_X + MAIN_W / 2, DISC_Y = ZONE_Y + 26
};

static SDL_Thread *s_thread;
static SDL_AtomicInt s_percent, s_stop, s_done, s_dialog; /* s_dialog: 2 once answered */
static char s_image[1200], s_answer[1200];
static void (*s_then)(const char *path);

static void centred(int y, Uint32 colour, const char *s) { text(MAIN_X + (MAIN_W - text_width(s, 1)) / 2, y, 1, colour, s); }

static void dashed(int x, int y, int w, int h, Uint32 colour)
{
    for (int i = 0; i < w; i += 8) {
        fill(x + i, y, SDL_min(4, w - i), 1, colour);
        fill(x + i, y + h, SDL_min(4, w - i), 1, colour);
    }
    for (int i = 0; i < h; i += 8) {
        fill(x, y + i, 1, SDL_min(4, h - i), colour);
        fill(x + w, y + i, 1, SDL_min(4, h - i), colour);
    }
}

/* a disc, 15 pixels round: silver, its rainbow's shine (turning while it's read, `turn` in eighths), the hole */
static void disc_icon(int cx, int cy, int turn)
{
    for (int dy = -15; dy <= 15; dy++) {
        for (int dx = -15; dx <= 15; dx++) {
            const int d2 = dx * dx + dy * dy;
            if (d2 > 15 * 15 || d2 < 3 * 3) {
                continue;
            }
            Uint32 c = d2 < 5 * 5 ? C_RULE : d2 > 14 * 14 ? C_EDGE : 0xB8CCD6;
            if (d2 >= 6 * 6 && d2 <= 13 * 13) { /* the shine: two opposite sectors */
                const int sector = (int)((SDL_atan2f((float)dy, (float)dx) + 3.14159f) / 6.28318f * 8 + turn) & 7;
                c = sector == 0 || sector == 4 ? C_LINK : sector == 1 || sector == 5 ? C_GOLD : c;
            }
            fill(cx + dx, cy + dy, 1, 1, c);
        }
    }
}

static void draw_button(const char *label, bool focused)
{
    fill(BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, focused ? C_GOLD_HI : C_GOLD);
    if (focused) {
        outline(BUTTON_X - 1, BUTTON_Y - 1, BUTTON_W + 2, BUTTON_H + 2, C_TEXT);
    }
    text_flat(BUTTON_X + (BUTTON_W - text_width(label, 1)) / 2, BUTTON_Y + 5, C_ON_SELECT, label);
}

void first_run_draw(void)
{
    const bool focused = rows()[g_app.sel[SC_HOME]].kind != R_LANGUAGE;
    text(MAIN_X, 12, 2, C_TEXT, L->s[S_WELCOME]);
    const int need = wrap(MAIN_X, 38, MAIN_W, C_TEXT, L->s[S_NEED]);
    text(MAIN_X, 38 + need * 12, 1, C_GOLD, GAME_LINE);
    wrap(MAIN_X, 50 + need * 12, MAIN_W, C_SOFT, L->s[S_ONCE]);
    panel(ZONE_X, ZONE_Y, ZONE_W, ZONE_H);
    dashed(ZONE_X + 6, ZONE_Y + 6, ZONE_W - 12, ZONE_H - 12, focused || s_thread ? C_LINK : C_EDGE);
    if (s_thread) {
        char pct[16];
        const int percent = SDL_GetAtomicInt(&s_percent), w = ZONE_W - 80;
        SDL_snprintf(pct, sizeof pct, "%d%%", percent);
        disc_icon(DISC_X, DISC_Y, (int)(SDL_GetTicks() / 120));
        centred(ZONE_Y + 47, C_TEXT, L->s[S_READING]);
        fill(ZONE_X + 40, ZONE_Y + 60, w, 8, C_BG);
        fill(ZONE_X + 40, ZONE_Y + 60, w * percent / 100, 8, C_LINK);
        outline(ZONE_X + 39, ZONE_Y + 59, w + 2, 10, C_EDGE);
        text(ZONE_X + 40 + w + 4, ZONE_Y + 60, 1, C_SOFT, pct);
        draw_button(L->s[S_CANCEL], true);
        return;
    }
    disc_icon(DISC_X, DISC_Y, 0);
    centred(ZONE_Y + 48, C_TEXT, L->s[S_DROP]);
    centred(ZONE_Y + 61, C_DIM, L->s[S_OR]);
    draw_button(L->s[S_BROWSE], focused);
    if (g_app.note) {
        wrap(MAIN_X, ZONE_Y + ZONE_H + 5, MAIN_W, C_SOFT, g_app.note);
    } else if (g_app.state == ST_ERROR) {
        const int lines = wrap(MAIN_X, ZONE_Y + ZONE_H + 5, MAIN_W, C_RED, L->s[g_app.error]);
        wrap(MAIN_X, ZONE_Y + ZONE_H + 5 + lines * 12, MAIN_W, C_DIM, L->s[S_SEE_LOG]);
    } else {
        wrap(MAIN_X, ZONE_Y + ZONE_H + 5, MAIN_W, C_DIM, L->s[S_ISO]);
    }
}

bool first_run_hit(int x, int y) { return x >= ZONE_X && x < ZONE_X + ZONE_W && y >= ZONE_Y && y < ZONE_Y + ZONE_H; }

bool first_run_button_hit(int x, int y) { return x >= BUTTON_X && x < BUTTON_X + BUTTON_W && y >= BUTTON_Y && y < BUTTON_Y + BUTTON_H; }

bool first_run_reading(void) { return s_thread != NULL; }

static int progress(int percent)
{
    SDL_SetAtomicInt(&s_percent, percent);
    return !SDL_GetAtomicInt(&s_stop);
}

static int SDLCALL read_thread(void *unused)
{
    (void)unused;
    const int result = native_extract(s_image, path_data(), progress);
    if (result == E_NONE) {
        sounds_render(); /* the viewer's original hit sounds, from the data just written */
    }
    SDL_SetAtomicInt(&s_done, 1);
    return result;
}

void first_run_image(const char *path)
{
    if (s_thread || !path || !path[0]) {
        return;
    }
    SDL_strlcpy(s_image, path, sizeof s_image);
    SDL_SetAtomicInt(&s_percent, 0);
    SDL_SetAtomicInt(&s_stop, 0);
    SDL_SetAtomicInt(&s_done, 0);
    g_app.note = NULL;
    picker_set_open(false);
    s_thread = SDL_CreateThread(read_thread, "extract", NULL);
}

static void SDLCALL answered(void *unused, const char *const *files, int filter)
{
    (void)unused;
    (void)filter;
    s_answer[0] = 0;
    if (files && files[0]) {
        SDL_strlcpy(s_answer, files[0], sizeof s_answer);
    }
    SDL_SetAtomicInt(&s_dialog, 2);
    SDL_Event e = {.type = SDL_EVENT_USER};
    SDL_PushEvent(&e);
}

void disc_ask(void (*then)(const char *path))
{
    static const SDL_DialogFileFilter FILTERS[] = {{"Disc images (.cue, .bin, .iso)", "cue;bin;iso"}};
    if (!s_thread && SDL_GetAtomicInt(&s_dialog) == 0) {
        s_then = then;
        SDL_SetAtomicInt(&s_dialog, 1);
        SDL_ShowOpenFileDialog(answered, NULL, g_app.window, FILTERS, 1, NULL, false);
    }
}

void first_run_browse(void) { disc_ask(first_run_image); }

int first_run_error(int e)
{
    static const short ERRORS[] = {[E_OPEN] = S_E_OPEN, [E_GAME] = S_E_GAME, [E_BAD] = S_E_BAD, [E_WRITE] = S_E_WRITE,
                                   [E_READ] = S_E_READ, [E_TRUNCATED] = S_E_TRUNCATED, [E_CUE] = S_E_CUE, [E_SPACE] = S_E_SPACE};
    return ERRORS[e];
}

static void finish(void)
{
    int result = E_WRITE;
    SDL_WaitThread(s_thread, &result);
    s_thread = NULL;
    if (result == E_NONE) {
        g_app.state = ST_READY;
        g_app.sel[SC_HOME] = 0;
    } else if (result == E_CANCELLED) {
        g_app.state = ST_NO_GAME;
    } else {
        g_app.state = ST_ERROR;
        g_app.error = first_run_error(result);
    }
}

void first_run_poll(void)
{
    if (SDL_GetAtomicInt(&s_dialog) == 2) {
        SDL_SetAtomicInt(&s_dialog, 0);
        s_then(s_answer);
    }
    if (s_thread && SDL_GetAtomicInt(&s_done)) {
        finish();
    }
}

/* cancels and waits: extract.c then leaves the data folder as it was */
void first_run_stop(void)
{
    SDL_SetAtomicInt(&s_stop, 1);
}

void first_run_wait(void)
{
    if (s_thread) {
        first_run_stop();
        finish();
    }
}
