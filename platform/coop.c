/* Co-op, phase 1: the other player on the field. For now (milestone 1) the
   "other player" is a ghost of the local one: FIELDSTG (patches:
   fieldstg/field.c) hands over the player's state every frame and gets it
   back as it was COOP_DELAY frames ago, to draw it with an actor of its own.
   The network will take the history's place later.

   The ghost's actor needs a place in VRAM for its frames, which the stages
   share out by hand. A field loads its sprite sheet as whole texture pages,
   the actors' slots inside them, so what was written says nothing; what is
   free is a part of the sheet left blank (all zero: colour 0, transparent)
   that no actor's slot covers. FIELDSTG reserves the slots, and
   native_coop_find_vram looks for such a box in the player's texture page.
   coop = 1 in native.ini turns it on. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "../include/native.h"
#include "coop_net.h"

int g_coop = 0; /* coop = 1: the ghost (main.c reads native.ini) */
int g_coop_debug = 0; /* DW3_COOP_DEBUG=1: the ghost's state in the log every 2 s (main.c) */

int g_exp_mult = 1;   /* exp_multiplier: the battle report's EXP times this (coop/patches stfgtrep/report.c) */
int g_money_mult = 1; /* money_multiplier: and its money */

int native_coop_ini(const char *key, const char *value) {
    if (SDL_strcasecmp(key, "coop") == 0) {
        g_coop = SDL_atoi(value) != 0;
    } else if (SDL_strcasecmp(key, "coop_peer") == 0 || SDL_strcasecmp(key, "coop_peers") == 0) {
        SDL_strlcpy(g_coop_peer, value, sizeof g_coop_peer);
        for (char *e = g_coop_peer + SDL_strlen(g_coop_peer); e > g_coop_peer && (e[-1] == ' ' || e[-1] == '\t');) {
            *--e = 0;
        }
    } else if (SDL_strcasecmp(key, "coop_port") == 0) {
        if (SDL_atoi(value) > 0 && SDL_atoi(value) < 65536) {
            g_coop_port = SDL_atoi(value);
        }
    } else if (SDL_strcasecmp(key, "exp_multiplier") == 0) {
        g_exp_mult = SDL_clamp(SDL_atoi(value), 1, 5);
    } else if (SDL_strcasecmp(key, "money_multiplier") == 0) {
        g_money_mult = SDL_clamp(SDL_atoi(value), 1, 5);
    } else if (SDL_strcasecmp(key, "heal_on_levelup") == 0 || SDL_strcasecmp(key, "save_anywhere") == 0 ||
               SDL_strcasecmp(key, "extra_packs") == 0) { /* extra_packs: the free team took the packs' place */
        /* options this build takes out: taken here, so they stay off whatever the file says */
    } else {
        return 0;
    }
    return 1;
}

/* --- VRAM: the slots kept for the field's actors --- */

static uint8_t s_reserved[VRAM_H][VRAM_W / 8];

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

/* --- The player's history: the ghost of milestone 1, without coop_peer --- */

#define COOP_DELAY 100 /* frames: 2 s at 50 Hz */
#define COOP_HISTORY (COOP_DELAY + 1)

static NativeCoopState s_history[COOP_HISTORY];
static int s_head, s_count;

/* The ghost of yourself, to test alone: DW3_COOP_GHOST=1 and no coop_peers. Players without addresses yet see nobody */
static int coop_ghost(void) {
    static int ghost = -1;

    if (ghost < 0) {
        ghost = g_coop_peer[0] == 0 && SDL_getenv("DW3_COOP_GHOST") != NULL;
    }
    return ghost;
}

void native_coop_reset(void) {
    s_head = 0;
    s_count = 0;
}

void native_coop_publish(const NativeCoopState *now) {
    if (coop_net_start()) {
        coop_net_send(now);
        coop_net_receive();
        return;
    }
    if (!coop_ghost()) {
        return; /* a coop_peer that didn't work (coop.log says why): nobody, not the ghost */
    }
    s_history[s_head] = *now;
    s_head = (s_head + 1) % COOP_HISTORY;
    if (s_count < COOP_HISTORY) {
        s_count++;
    }
}

/* How many other players there can be: coop_peers' (found), or 1, the ghost, without it */
int native_coop_peer_slots(void) {
    return g_coop_net_on ? coop_net_peer_count() : coop_ghost();
}

int native_coop_peer(int i, NativeCoopState *out) {
    if (g_coop_net_on) {
        return coop_net_peer(i, out);
    }
    if (i != 0 || !coop_ghost() || s_count < COOP_HISTORY) {
        return 0;
    }
    static int offset = -1;

    *out = s_history[s_head]; /* the oldest: COOP_DELAY frames ago */
    if (offset < 0) {
        /* DW3_COOP_OFFSET=N: the ghost N tiles to the right, to see it with the player standing still (tests) */
        const char *env = SDL_getenv("DW3_COOP_OFFSET");
        offset = env != NULL ? SDL_atoi(env) : 0;
    }
    out->x += offset << 8;
    return 1;
}
