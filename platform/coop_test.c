/* Co-op: the helpers of our own tests (scripts/coop_test.ps1), by environment variables: not for players */
#include <stdio.h>
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "../include/native.h"
#include "coop_net.h"

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

int native_coop_start_delay(void) {
    const char *env = SDL_getenv("DW3_COOP_START_DELAY");

    return env != NULL ? SDL_atoi(env) : 0;
}

/* DW3_COOP_STOMP=<frames> (tests): 1 once, that many calls after the first (the Guardromon's window blanks its lilac
   CLUTs then, as a map loading over them would) */
int native_coop_test_stomp(void) {
    static int calls = -1;
    const char *env = SDL_getenv("DW3_COOP_STOMP");

    if (env == NULL || calls == -2) {
        return 0;
    }
    if (++calls < SDL_atoi(env)) {
        return 0;
    }
    calls = -2;
    return 1;
}

int native_coop_raid_test(void) {
    const char *env = SDL_getenv("DW3_COOP_RAID");

    return env != NULL ? (int)SDL_strtol(env, NULL, 0) : 0;
}

/* DW3_COOP_AUTO=trade or battle (tests): a field waits at a Guardromon for that by itself, once; -1 none */
int native_coop_auto(void) {
    static int done;
    const char *env = SDL_getenv("DW3_COOP_AUTO");

    if (done || env == NULL) {
        return -1;
    }
    done = 1;
    return SDL_strcmp(env, "duel") == 0 ? 4 : SDL_strcmp(env, "menu") == 0 ? 3 : SDL_strcmp(env, "raid") == 0 ? 2 : SDL_strcmp(env, "battle") == 0 || SDL_strcmp(env, "giveup") == 0 || SDL_strcmp(env, "tech") == 0;
}

int native_coop_auto_battle(void) {
    static int on = -1;

    if (on < 0) {
        const char *env = SDL_getenv("DW3_COOP_AUTO");

        on = env == NULL ? 0 : SDL_strcmp(env, "battle") == 0 || SDL_strcmp(env, "raid") == 0 || SDL_strcmp(env, "duel") == 0 ? 1 : SDL_strcmp(env, "giveup") == 0 ? 2 : SDL_strcmp(env, "tech") == 0 ? 3 : 0;
    }
    return on;
}

static int s_press_frame, s_quiet_frame;

int native_coop_auto_press(void) {
    ++s_press_frame;
    return native_coop_lockstep() && native_coop_auto_battle() && s_press_frame - s_quiet_frame > 2 && s_press_frame % 40 < 4;
}

void native_coop_auto_quiet(void) {
    s_quiet_frame = s_press_frame;
}

/* DW3_TEST="digilog,...": what scripts/probar.ps1 wants; each word a test's (state.c, ststatus/item_list.c) */
int native_test(const char *what) {
    const char *env = SDL_getenv("DW3_TEST");
    size_t n = SDL_strlen(what);

    for (const char *p = env; p != NULL && (p = SDL_strstr(p, what)) != NULL; p += n) {
        if ((p == env || p[-1] == ',') && (p[n] == 0 || p[n] == ',')) {
            return 1;
        }
    }
    return 0;
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
