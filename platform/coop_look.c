/* Co-op: the other players' look. The character creator's model and tone, and the VRAM their frames take: a
   field loads its sprite sheet as whole texture pages, the actors' slots inside them, so what was written says
   nothing; what is free is a part of the sheet left blank (all zero: colour 0, transparent) that no actor's slot
   covers. FIELDSTG reserves the slots, and native_coop_find_vram looks for such a box in the player's texture page. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "../include/native.h"
#include "coop_net.h"

/* The player's model (character creator): a character (Actor.key1) whose look the player takes, 0 Junpei's own.
   For now DW3_PLAYER_MODEL=<key1, hex> (tests) */
int native_player_model(void) {
    static int model = -1;

    if (model < 0) {
        const char *env = SDL_getenv("DW3_PLAYER_MODEL");

        model = env != NULL ? (int)SDL_strtol(env, NULL, 16) : 0;
        if (model < 0 || model > 0x1FF) {
            model = 0;
        }
    }
    return model;
}

/* DW3_PLAYER_TONE=<0-7> (tests): the tone with DW3_PLAYER_MODEL, the save's otherwise; -1 none set */
int native_player_tone(void) {
    static int tone = -2;

    if (tone == -2) {
        const char *env = SDL_getenv("DW3_PLAYER_TONE");

        tone = env != NULL ? SDL_atoi(env) & 7 : -1;
    }
    return tone;
}

/* --- VRAM: the slots kept for the field's actors --- */

static uint8_t s_reserved[VRAM_H][VRAM_W / 8];
#define COOP_MINE 8

static struct {
    int x, y, w, h, live;
    uint32_t hash;
} s_mine[COOP_MINE];

static uint32_t box_hash(int x, int y, int w, int h) {
    uint32_t hash = 2166136261u;

    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            hash = (hash ^ g_vram[j * VRAM_W + i]) * 16777619u;
        }
    }
    return hash;
}


static int taken(int x, int y, int w, int h) {
    int i, j;

    for (j = y; j < y + h; j++) {
        for (i = x; i < x + w; i++) {
            if ((s_reserved[j][i >> 3] & (1 << (i & 7))) || g_vram[j * VRAM_W + i] != 0) {
                return 1;
            }
        }
    }
    return 0;
}

void native_coop_vram_reset(void) {
    memset(s_reserved, 0, sizeof s_reserved);
    /* as the field starts, before its stage loads: the last map's boxes as it left them, for native_coop_vram_reclaim */
    for (int k = 0; k < COOP_MINE; k++) {
        if (s_mine[k].w != 0) {
            s_mine[k].hash = box_hash(s_mine[k].x, s_mine[k].y, s_mine[k].w, s_mine[k].h);
            s_mine[k].live = 0;
        }
    }
}

void native_coop_vram_reserve(int x, int y, int w, int h) {
    int i, j;

    for (j = y < 0 ? 0 : y; j < y + h && j < VRAM_H; j++) {
        for (i = x < 0 ? 0 : x; i < x + w && i < VRAM_W; i++) {
            s_reserved[j][i >> 3] |= (uint8_t)(1 << (i & 7));
        }
    }
}

/* A w x h box (in VRAM's 16-bit words) inside the texture page at (page_x,
   page_y), 64 words by 256 rows for 4-bit textures, blank and kept for no
   slot: 1 with its corner in *out_x, *out_y. Searched from the page's bottom
   right, away from where the stages pack their slots. */
int native_coop_find_vram(int page_x, int page_y, int w, int h, int *out_x, int *out_y) {
    int x, y;


    for (y = page_y + 256 - h; y >= page_y; y -= 4) {
        for (x = page_x + 64 - w; x >= page_x; x -= 2) {
            if (x >= 0 && y >= 0 && x + w <= VRAM_W && y + h <= VRAM_H && !taken(x, y, w, h)) {
                *out_x = x;
                *out_y = y;
                return 1;
            }
        }
    }
    return 0;
}

/* The same in any texture page right of the frame buffers (x 640 on, where a
   black screen would look blank): 1 with the box's corner and its page */
int native_coop_find_vram_any(int w, int h, int *out_x, int *out_y, int *page_x, int *page_y) {
    int px, py;

    for (py = 0; py < VRAM_H; py += 256) {
        for (px = VRAM_W - 64; px >= 640; px -= 64) {
            if (native_coop_find_vram(px, py, w, h, out_x, out_y)) {
                *page_x = px;
                *page_y = py;
                return 1;
            }
        }
    }
    return 0;
}

/* The boxes the other players' frames took, map after map: a box must be blank to be taken, and a map leaves its
   players' frames behind, so each map would take a new one till none was left. A box of the last map whose words are
   as they were when it was left (the new map put nothing there) is blanked for the new map's players to take again */


void native_coop_vram_mine(int x, int y, int w, int h) {
    int k, free = -1;

    for (k = 0; k < COOP_MINE; k++) {
        if (s_mine[k].w == 0 || (s_mine[k].x == x && s_mine[k].y == y)) {
            free = k;
            if (s_mine[k].w != 0) {
                break;
            }
        }
    }
    if (free < 0) {
        free = 0; /* full: the oldest goes */
        SDL_memmove(&s_mine[0], &s_mine[1], sizeof s_mine[0] * (COOP_MINE - 1));
        free = COOP_MINE - 1;
    }
    s_mine[free].x = x;
    s_mine[free].y = y;
    s_mine[free].w = w;
    s_mine[free].h = h;
    s_mine[free].live = 1;
    s_mine[free].hash = box_hash(x, y, w, h);
}

void native_coop_vram_reclaim(void) {
    for (int k = 0; k < COOP_MINE; k++) {
        if (s_mine[k].w == 0 || s_mine[k].live) {
            continue;
        }
        int x = s_mine[k].x, y = s_mine[k].y, w = s_mine[k].w, h = s_mine[k].h;

        if (box_hash(x, y, w, h) == s_mine[k].hash) {
            soft_write(x, y, w, h);
            for (int j = 0; j < h; j++) {
                memset(&g_vram[(y + j) * VRAM_W + x], 0, (size_t)w * 2);
            }
            gl_vram_dirty(x, y, w, h);
            gl_refresh(x, y, w, h);
        }
        /* kept either way: a box another map loaded over (another lead's frames, say) is cleared again by the first map
           after it that leaves it as it was, instead of being lost for good */
    }
}

/* Copies w x h words of VRAM, for a CLUT the ghost takes along */
void native_coop_vram_copy(int from_x, int from_y, int to_x, int to_y, int w, int h) {
    int j;

    soft_write(to_x, to_y, w, h);
    for (j = 0; j < h; j++) {
        memmove(&g_vram[(to_y + j) * VRAM_W + to_x], &g_vram[(from_y + j) * VRAM_W + from_x], (size_t)w * 2);
    }
    gl_vram_dirty(to_x, to_y, w, h);
    gl_refresh(to_x, to_y, w, h);
}

/* Turns the colours of the 16-colour CLUT at (x, y) for other player n (0-2), so the players tell each other apart:
   blue to red, to green, to purple. The other colours (skin, greys, the transparent 0) stay. */
void native_coop_vram_tint(int x, int y, int n) {
    uint16_t *clut = &g_vram[y * VRAM_W + x];
    int i;

    soft_write(x, y, 16, 1);
    for (i = 0; i < 16; i++) {
        int c = clut[i], r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        int hi = SDL_max(r, SDL_max(g, b)), lo = SDL_min(r, SDL_min(g, b)), nr, ng, nb;

        /* only the blues (hair, clothes): what it makes isn't blue any more, so turning it again changes nothing */
        if (c == 0 || hi - lo <= 3 || b <= r || b <= g) {
            continue;
        }
        switch (n) {
        case 0: nr = b; ng = g; nb = r; break;           /* red and blue swapped */
        case 1: nr = r; ng = b; nb = g; break;           /* green and blue swapped */
        default: nr = (b * 3) / 4; ng = g; nb = b; break; /* blue with red: purple */
        }
        clut[i] = (uint16_t)((c & 0x8000) | nr | (ng << 5) | (nb << 10));
    }
    gl_vram_dirty(x, y, 16, 1);
    gl_refresh(x, y, 16, 1);
}

/* The character creator's tones: a 16-colour CLUT's colours turned for tone (0 as drawn, 1-7 the hue turned by
   45 degrees each), from src to dst (both BGR555, the semi-transparency bit kept). Greys, black, white and skin (light,
   a little saturated, between red and yellow) stay */
void native_tone_clut(uint16_t *dst, const uint16_t *src, int tone) {
    int i;

    for (i = 0; i < 16; i++) {
        int c = src[i], r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        int hi = SDL_max(r, SDL_max(g, b)), lo = SDL_min(r, SDL_min(g, b)), sat = hi - lo, hue, h6, f, p, q, t;

        dst[i] = (uint16_t)c;
        if (tone <= 0 || c == 0 || sat <= 3) {
            continue;
        }
        /* the hue, 0-359 */
        hue = hi == r ? 60 * (g - b) / sat : hi == g ? 120 + 60 * (b - r) / sat : 240 + 60 * (r - g) / sat;
        hue = (hue + 360) % 360;
        if (hue >= 5 && hue <= 50 && hi >= 22 && sat <= 16) {
            continue; /* skin */
        }
        hue = (hue + tone * 45) % 360;
        /* back to RGB at the same value and saturation */
        h6 = hue / 60;
        f = hue % 60;
        p = lo;
        q = hi - sat * f / 60;
        t = lo + sat * f / 60;
        switch (h6) {
        case 0: r = hi; g = t; b = p; break;
        case 1: r = q; g = hi; b = p; break;
        case 2: r = p; g = hi; b = t; break;
        case 3: r = p; g = q; b = hi; break;
        case 4: r = t; g = p; b = hi; break;
        default: r = hi; g = p; b = q; break;
        }
        dst[i] = (uint16_t)((c & 0x8000) | r | (g << 5) | (b << 10));
    }
}

/* Writes a 16-colour CLUT at (x, y) in VRAM */
void native_vram_clut(int x, int y, const uint16_t *colors) {
    soft_write(x, y, 16, 1);
    memcpy(&g_vram[y * VRAM_W + x], colors, 32);
    gl_vram_dirty(x, y, 16, 1);
    gl_refresh(x, y, 16, 1);
}

/* The page as text, one character per 4 x 8 words: '.' free, '#' taken (when
   no box was found, to see why) */
void native_coop_vram_dump(int page_x, int page_y) {
    char line[17];
    int x, y;

    for (y = page_y; y < page_y + 256 && y < VRAM_H; y += 8) {
        for (x = 0; x < 16; x++) {
            line[x] = taken(page_x + x * 4, y, 4, 8) ? '#' : '.';
        }
        line[16] = 0;
        LOG("coop: vram %3d %s", y, line);
    }
}

/* --- the character creator's look over the network --- */

int native_coop_model_ok(int model) {
    return model == 0 || model == 0x31 || model == 0x10; /* actor_control.c's COOP_LOOK_MODELS */
}
