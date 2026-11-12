/* Co-op: each other player's name above their head (platform/video.c draws them over the frame) */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "../include/native.h"
#include "coop_net.h"

/* GAME.name's bytes (the game's text: A 0x0E.., a 0x28.., 0 0x04.., two-byte signs after 0x01) to ASCII */
static void coop_name_ascii(const char *name, char *out, int size) {
    const uint8_t *b = (const uint8_t *)name;
    int n = 0;

    for (int i = 0; i < 16 && b[i] != 0 && n < size - 1; i++) {
        int c = b[i];
        char ch = 0;

        if (c >= 0x04 && c <= 0x0D) {
            ch = (char)('0' + c - 0x04);
        } else if (c >= 0x0E && c <= 0x27) {
            ch = (char)('A' + c - 0x0E);
        } else if (c >= 0x28 && c <= 0x41) {
            ch = (char)('a' + c - 0x28);
        } else if (c == 0xE6) {
            ch = '?';
        } else if (c == 0xE7) {
            ch = '!';
        } else if ((c == 0x01 || c == 0x02) && i + 1 < 16) {
            int d = b[++i];

            ch = c == 0x02 ? 0 : d == 0x01 ? ' ' : d == 0x04 ? ',' : d == 0x05 ? '.' : d == 0x07 ? ':' : d == 0x13 ? '\''
               : d == 0x1A ? '-' : '~';
        }
        if (ch != 0) {
            out[n++] = ch;
        }
    }
    out[n] = 0;
}

#define COOP_TAGS 3
static struct {
    int x, y;
    Uint64 at; /* SDL ticks it was drawn: the tag shows while it's fresh */
} s_tags[COOP_TAGS];

void native_coop_tag(int n, int x, int y) {
    if (n >= 0 && n < COOP_TAGS) {
        s_tags[n].x = x;
        s_tags[n].y = y;
        s_tags[n].at = SDL_GetTicks();
    }
}

static int s_tags_off; /* a card trade's window up (fieldstg/field.c): no tags over it */

void native_coop_tags_block(int on) {
    s_tags_off = on;
}

static int tag_fresh(int n) {
    return !s_tags_off && s_tags[n].at != 0 && SDL_GetTicks() - s_tags[n].at < 100;
}

int native_coop_tags_due(void) {
    for (int n = 0; n < COOP_TAGS; n++) {
        if (tag_fresh(n)) {
            return 1;
        }
    }
    return 0;
}

/* Over the frame, in 320x240: each player's name above their head, on a dark plate with their colour's stripe */
void native_coop_draw_tags(SDL_Renderer *r) {
    static const uint8_t COLOURS[COOP_TAGS][3] = {{240, 90, 70}, {110, 220, 110}, {190, 120, 240}} /* the tints */;

    for (int n = 0; n < COOP_TAGS; n++) {
        NativeCoopState st;
        char text[20];

        if (!tag_fresh(n) || !native_coop_peer(n, &st)) {
            continue;
        }
        coop_name_ascii(st.name, text, sizeof text);
        if (text[0] == 0) {
            SDL_snprintf(text, sizeof text, "P%d", n + 2);
        }
        float w = 8.0f * (float)SDL_strlen(text) + 6, h = 12;
        SDL_FRect plate = {(float)s_tags[n].x - w / 2, (float)s_tags[n].y - h - 3, w, h};

        plate.x = SDL_clamp(plate.x, 1, 319 - w);
        plate.y = SDL_clamp(plate.y, 1, 239 - h);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 150);
        SDL_RenderFillRect(r, &plate);
        SDL_FRect stripe = {plate.x, plate.y + h - 1, w, 1};

        SDL_SetRenderDrawColor(r, COLOURS[n][0], COLOURS[n][1], COLOURS[n][2], 255);
        SDL_RenderFillRect(r, &stripe);
        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        SDL_RenderDebugText(r, plate.x + 3, plate.y + 2, text);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}
