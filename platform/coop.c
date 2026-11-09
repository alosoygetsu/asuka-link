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
        s_mine[k].w = 0; /* blanked, or the map's own now */
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

/* --- The player's history: the ghost of milestone 1, without coop_peer --- */

#define COOP_DELAY 100 /* frames: 2 s at 50 Hz */
#define COOP_HISTORY (COOP_DELAY + 1)

static NativeCoopState s_history[COOP_HISTORY];
static NativeCoopState s_last; /* the field's state last published: in a battle the packets carry it, off the field */
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
    s_last = *now;
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

/* --- the character creator's look over the network, and the name tags --- */

int native_coop_model_ok(int model) {
    return model == 0 || model == 0x31 || model == 0x10; /* actor_control.c's COOP_LOOK_MODELS */
}

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

/* The character whose talk offers the card trade: Guardromon (0x15), or DW3_COOP_GUARD's (tests, hex) */
int native_coop_guard_key(void) {
    static int key = -1;

    if (key < 0) {
        const char *env = SDL_getenv("DW3_COOP_GUARD");

        key = env != NULL ? (int)SDL_strtol(env, NULL, 16) : 0x15;
    }
    return key;
}

/* DW3_COOP_CARDS=n (tests): a field gives n copies of the album's first cards once, to have spare ones to trade */
int native_coop_test_cards(void) {
    static int done;
    const char *env = SDL_getenv("DW3_COOP_CARDS");

    if (done || env == NULL) {
        return 0;
    }
    done = 1;
    return SDL_atoi(env);
}

/* --- the PvP battle's state, kept from the field to the battle and back --- */

static NativeCoopPvp s_pvp;

NativeCoopPvp *native_coop_pvp(void) {
    return s_pvp.on ? &s_pvp : NULL;
}

void native_coop_pvp_begin(int peer, const void *team, int size, const void *save, int battle, int first) {
    SDL_memset(&s_pvp, 0, sizeof s_pvp);
    SDL_memcpy(s_pvp.team, team, SDL_min(size, (int)sizeof s_pvp.team));
    SDL_memcpy(s_pvp.save, save, sizeof s_pvp.save);
    s_pvp.peer = peer;
    s_pvp.battle = battle & 0xFFFF;
    s_pvp.first = first;
    s_pvp.on = 1;
    native_coop_commands_out(peer, s_pvp.out);
}

void native_coop_pvp_end(void) {
    s_pvp.on = 0;
    SDL_memset(s_pvp.out, 0, sizeof s_pvp.out);
    native_coop_commands_out(s_pvp.peer, s_pvp.out);
}

void native_coop_pvp_pump(void) {
    NativeCoopState off = s_last;

    off.mode = -1; /* nobody draws this player while it battles */
    if (coop_net_start()) {
        coop_net_send(&off);
        coop_net_receive();
    }
}

void native_coop_pvp_send(NativeCoopCommand *c) {
    c->battle = (unsigned short)s_pvp.battle;
    c->number = (unsigned short)++s_pvp.sent;
    SDL_memmove(&s_pvp.out[1], &s_pvp.out[0], sizeof s_pvp.out[0] * (COOP_COMMANDS - 1));
    s_pvp.out[0] = *c;
    native_coop_commands_out(s_pvp.peer, s_pvp.out);
    if (1) { /* each command in coop.log: what a stuck battle was waiting for */
        native_coop_note("PvP: sent command %d: action %d arg %d digimon %d%s", c->number, c->action, c->arg,
                         c->digimon, c->knockOut ? " (after a knockout)" : "");
    }
}

int native_coop_pvp_take(NativeCoopCommand *c) {
    NativeCoopCommand in[COOP_COMMANDS];
    int n;
    unsigned int now = SDL_GetTicks();

    if (!s_pvp.on) {
        return -1;
    }
    n = native_coop_commands_in(s_pvp.peer, in);
    if (s_pvp.waitFrom == 0) {
        s_pvp.waitFrom = now;
    }
    if (n == 0) {
        /* the other silent: gone, after a while (a battle's loading is silent too, a moment) */
        if (now - s_pvp.waitFrom > 8000) {
            s_pvp.over = 2;
            native_coop_note("PvP: player %d is gone", s_pvp.peer + 2);
            return -1;
        }
        return 0;
    }
    for (int i = 0; i < n; i++) {
        if (in[i].battle == (unsigned short)s_pvp.battle && in[i].number == s_pvp.taken + 1) {
            *c = in[i];
            s_pvp.taken++;
            s_pvp.waitFrom = 0;
            if (c->action == 1) {
                s_pvp.over = 1;
            }
            if (1) {
                native_coop_note("PvP: took command %d: action %d arg %d digimon %d%s", c->number, c->action, c->arg,
                                 c->digimon, c->knockOut ? " (after a knockout)" : "");
            }
            return 1;
        }
    }
    s_pvp.waitFrom = s_pvp.waitFrom != 0 ? s_pvp.waitFrom : now;
    if (now - s_pvp.waitFrom > 8000) {
        /* here, but not in a battle any more: back on the field (a battle sends mode -1) */
        NativeCoopState st;

        if (native_coop_peer(s_pvp.peer, &st) && st.mode != -1) {
            s_pvp.over = 2;
            native_coop_note("PvP: player %d left the battle", s_pvp.peer + 2);
            return -1;
        }
    }
    return 0;
}

/* the battle's rolls: a struct laid out as the game's Random (include/dw3/random.h) */
struct Random {
    int index;
    void (*seed)(int seed);
    int (*next)(void);
};

static unsigned int s_roll = 1;

static int pvp_roll(void) {
    s_roll = s_roll * 1103515245u + 12345u;
    return (int)((s_roll >> 16) & 0xFFF); /* as RANDOM's: 0 to RANDOM_COUNT - 1 */
}

static struct Random s_pvp_random = {0, NULL, pvp_roll};

struct Random *native_coop_random(struct Random *game) {
    return s_pvp.on ? &s_pvp_random : game;
}

void native_coop_pvp_seed(unsigned int seed) {
    s_roll = seed;
}

/* DW3_COOP_AUTO=trade or battle (tests): a field waits at a Guardromon for that by itself, once; -1 none */
int native_coop_auto(void) {
    static int done;
    const char *env = SDL_getenv("DW3_COOP_AUTO");

    if (done || env == NULL) {
        return -1;
    }
    done = 1;
    return SDL_strcmp(env, "battle") == 0 || SDL_strcmp(env, "giveup") == 0;
}

int native_coop_auto_battle(void) {
    static int on = -1;

    if (on < 0) {
        const char *env = SDL_getenv("DW3_COOP_AUTO");

        on = env == NULL ? 0 : SDL_strcmp(env, "battle") == 0 ? 1 : SDL_strcmp(env, "giveup") == 0 ? 2 : 0;
    }
    return on;
}

static int s_press_frame, s_quiet_frame;

int native_coop_auto_press(void) {
    ++s_press_frame;
    return s_pvp.on && native_coop_auto_battle() && s_press_frame - s_quiet_frame > 2 && s_press_frame % 40 < 4;
}

void native_coop_auto_quiet(void) {
    s_quiet_frame = s_press_frame;
}

unsigned int native_coop_coin(void) {
    Uint64 t = SDL_GetPerformanceCounter();

    return (unsigned int)(t ^ (t >> 32)) * 2654435761u ^ (unsigned int)g_coop_port * 40503u;
}

void native_coop_test_save(void *game) {
    const char *env = SDL_getenv("DW3_COOP_SAVE");
    char path[512];
    const char *at;
    FILE *f;

    if (env == NULL || (at = SDL_strchr(env, '@')) == NULL || at - env >= (int)sizeof path) {
        return;
    }
    SDL_memcpy(path, env, at - env);
    path[at - env] = 0;
    if ((f = fopen(path, "rb")) != NULL) {
        if (fseek(f, SDL_strtol(at + 1, NULL, 0), SEEK_SET) == 0 && fread(game, COOP_PVP_SAVE, 1, f) == 1) {
            native_coop_note("test: the save at %s", env);
        }
        fclose(f);
    }
}
