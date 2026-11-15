/* Co-op: the room's notices while playing. The launcher, minimised, keeps the room going and writes sala_aviso.txt
   next to the game (launcher/sala.c): "I id seq" an invitation, "N 0 seq" someone who joined or left, then two lines
   in ASCII (the debug font's). This shows it at the top of the frame (video.c draws the overlay); an invitation is
   accepted holding SELECT a second, which writes its id to sala_respuesta.txt for the launcher. Not answering lets it
   go (30 s, as in the launcher). */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include "platform.h"

#define READ_MS 500    /* a look at the file */
#define HOLD_MS 1000   /* SELECT held this long: yes */
#define NOTE_MS 6000   /* a join or leave shows this long */
#define INVITE_MS 30000

static char s_kind; /* 'I', 'N' or 0 */
static unsigned s_id, s_seq;
static char s_line[2][48];
static Uint64 s_next_read, s_seen_at, s_held_since;
static int s_answered, s_was_held;

static void file_path(char *out, int size, const char *name) {
    SDL_snprintf(out, size, "%s%s", SDL_GetBasePath(), name);
}

static void cut_line(char *s) {
    s[strcspn(s, "\r\n")] = 0;
}

static void notice_read(void) {
    char path[1100], head[64], line[2][48] = {"", ""};
    char kind = 0;
    unsigned id = 0, seq = 0;
    FILE *f;

    if (SDL_GetTicks() < s_next_read) {
        return;
    }
    s_next_read = SDL_GetTicks() + READ_MS;
    file_path(path, sizeof path, "sala_aviso.txt");
    if ((f = fopen(path, "r")) == NULL) {
        s_kind = 0;
        return;
    }
    if (fgets(head, sizeof head, f) == NULL || sscanf(head, " %c %x %u", &kind, &id, &seq) != 3 || (kind != 'I' && kind != 'N')) {
        kind = 0;
    }
    for (int i = 0; i < 2 && fgets(line[i], sizeof line[i], f) != NULL; i++) {
        cut_line(line[i]);
    }
    fclose(f);
    if (kind != s_kind || seq != s_seq) { /* a new one */
        s_kind = kind;
        s_id = id;
        s_seq = seq;
        memcpy(s_line, line, sizeof s_line);
        s_seen_at = SDL_GetTicks();
        s_answered = 0;
        s_held_since = 0;
        s_was_held = 1; /* a SELECT already down doesn't count: it has to be pressed after it shows */
        if (kind) {
            LOG("room notice: %c %s / %s", kind, line[0], line[1]);
        }
    }
}

static void answer(void) {
    char path[1100], tmp[1200], text[16];

    file_path(path, sizeof path, "sala_respuesta.txt");
    SDL_snprintf(tmp, sizeof tmp, "%s.tmp", path);
    SDL_snprintf(text, sizeof text, "%08x\n", s_id);
    if (SDL_SaveFile(tmp, text, SDL_strlen(text)) && SDL_RenamePath(tmp, path)) {
        s_answered = 1;
        LOG("room notice: invitation %08x accepted", s_id);
    }
}

/* SELECT, from its press: held long enough, the yes */
static void watch_select(void) {
    const int held = (pad_held() & 1) != 0;

    if (s_answered) {
        return;
    }
    if (held && !s_was_held) {
        s_held_since = SDL_GetTicks();
    } else if (!held) {
        s_held_since = 0;
    }
    s_was_held = held;
    if (s_held_since != 0 && SDL_GetTicks() - s_held_since >= HOLD_MS) {
        answer();
    }
}

int native_coop_notice_due(void) {
    notice_read();
    const Uint64 age = SDL_GetTicks() - s_seen_at;

    if (s_kind == 'N') {
        return age < NOTE_MS;
    }
    if (s_kind == 'I' && age < INVITE_MS) {
        watch_select();
        return 1;
    }
    return 0;
}

/* in 320x240: a dark plate at the top, a gold edge, the bar filling while SELECT is held */
void native_coop_draw_notice(SDL_Renderer *r) {
    const int lines = s_line[1][0] ? 2 : 1, bar = s_kind == 'I' && !s_answered;
    const int len = (int)SDL_max(SDL_strlen(s_line[0]), SDL_strlen(s_line[1]));
    const float w = 8.0f * (float)len + 12, h = 10.0f * (float)lines + 6 + (bar ? 5 : 0);
    SDL_FRect plate = {(320 - w) / 2, 6, w, h};
    SDL_FRect edge = {plate.x, plate.y, w, 1}, edge2 = {plate.x, plate.y + h - 1, w, 1};

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 0, 0, 0, 190);
    SDL_RenderFillRect(r, &plate);
    SDL_SetRenderDrawColor(r, 240, 200, 80, 255);
    SDL_RenderFillRect(r, &edge);
    SDL_RenderFillRect(r, &edge2);
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderDebugText(r, plate.x + 6, plate.y + 4, s_line[0]);
    if (lines > 1) {
        if (s_kind == 'I') {
            SDL_SetRenderDrawColor(r, s_answered ? 140 : 240, s_answered ? 140 : 200, s_answered ? 140 : 80, 255);
        }
        SDL_RenderDebugText(r, plate.x + 6, plate.y + 14, s_line[1]);
    }
    if (bar) {
        const float full = w - 12, t = s_held_since ? (float)(SDL_GetTicks() - s_held_since) / HOLD_MS : 0;
        SDL_FRect back = {plate.x + 6, plate.y + h - 6, full, 2}, done = back;

        done.w = full * SDL_clamp(t, 0.0f, 1.0f);
        SDL_SetRenderDrawColor(r, 110, 100, 70, 255);
        SDL_RenderFillRect(r, &back);
        SDL_SetRenderDrawColor(r, 240, 200, 80, 255);
        SDL_RenderFillRect(r, &done);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
