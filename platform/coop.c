/* Co-op: the other players on the field. FIELDSTG (patches: fieldstg/field.c) hands over the player's state
   every frame (native_coop_publish) and draws each other player with an actor of its own (native_coop_peer): from
   the network (coop_net.c), or, to test alone, a ghost of the local one as it was COOP_DELAY frames ago.
   coop = 1 in native.ini turns it on. The rest is apart: coop_look.c the other players' look (VRAM, tints, tones),
   coop_tags.c their name tags, coop_battle.c the battles in lockstep (PvP, raids, duels), coop_test.c the tests' helpers. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "../include/native.h"
#include "coop_net.h"

int g_coop = 1; /* coop = 0 in native.ini turns it off; on by default: without coop_peers it opens no socket */
int g_coop_debug = 0; /* DW3_COOP_DEBUG=1: the ghost's state in the log every 2 s (main.c) */

int g_exp_mult = 1;   /* exp_multiplier: the battle report's EXP times this (coop/patches stfgtrep/report.c) */
int g_money_mult = 1; /* money_multiplier: and its money */
int g_sure_training = 1; /* sure_training: the gyms without luck: every try works, the bonus one too, the most of each gain and no stat lowered */


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
    } else if (SDL_strcasecmp(key, "sure_training") == 0) {
        g_sure_training = SDL_atoi(value) != 0;
    } else if (SDL_strcasecmp(key, "heal_on_levelup") == 0 || SDL_strcasecmp(key, "save_anywhere") == 0 ||
               SDL_strcasecmp(key, "extra_packs") == 0) { /* removed options: an old native.ini's lines are skipped */
        /* options this build takes out: taken here, so they stay off whatever the file says */
    } else {
        return 0;
    }
    return 1;
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

/* The character whose talk offers the card trade: Guardromon (0x15), or DW3_COOP_GUARD's (tests, hex) */
int native_coop_guard_key(void) {
    static int key = -1;

    if (key < 0) {
        const char *env = SDL_getenv("DW3_COOP_GUARD");

        key = env != NULL ? (int)SDL_strtol(env, NULL, 16) : 0x15;
    }
    return key;
}

/* A battle in lockstep: this player is off the field (mode -1), and still says so each frame */
void native_coop_pvp_pump(void) {
    NativeCoopState off = s_last;

    off.mode = -1; /* nobody draws this player while it battles */
    if (coop_net_start()) {
        coop_net_send(&off);
        coop_net_receive();
    }
}
