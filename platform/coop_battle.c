/* Co-op: the battles in lockstep, PvP, raids and card duels: their state from the field to the battle and back,
   the commands each side sends (coop_net.c carries them), the shared rolls and the raid's reward */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <string.h>
#include "platform.h"
#include "../include/native.h"
#include "coop_net.h"

/* --- the PvP battle's state, kept from the field to the battle and back --- */

static NativeCoopPvp s_pvp;

NativeCoopPvp *native_coop_pvp(void) {
    return s_pvp.on && !s_pvp.raid && !s_pvp.duel ? &s_pvp : NULL;
}

NativeCoopPvp *native_coop_duel(void) {
    return s_pvp.on && s_pvp.duel ? &s_pvp : NULL;
}

/* The card duel's messages (cardgame/): each one numbered, the newest DUEL_MSGS going out as one blob (re-sent whole on
   each new one), the other's taken in order */
#define DUEL_MSGS 12
#define DUEL_DATA 232
typedef struct DuelMsg {
    uint16_t seq, kind, size, pad;
    uint8_t data[DUEL_DATA];
} DuelMsg;

static DuelMsg s_duelOut[DUEL_MSGS];
static int s_duelSeq, s_duelTaken, s_duelBlob;

void native_coop_duel_set(void) {
    s_pvp.duel = 1;
    SDL_memset(s_duelOut, 0, sizeof s_duelOut);
    s_duelSeq = s_duelTaken = 0;
    s_duelBlob = 0;
}

void native_coop_duel_post(int kind, const void *data, int size) {
    DuelMsg *m;

    SDL_memmove(&s_duelOut[1], &s_duelOut[0], sizeof s_duelOut[0] * (DUEL_MSGS - 1));
    m = &s_duelOut[0];
    SDL_memset(m, 0, sizeof *m);
    m->seq = (uint16_t)++s_duelSeq;
    m->kind = (uint16_t)kind;
    m->size = (uint16_t)SDL_min(size, DUEL_DATA);
    SDL_memcpy(m->data, data, m->size);
    s_duelBlob = (s_duelBlob % 0x3FFF) + 1;
    native_coop_blob_send(s_pvp.peer, s_duelOut, sizeof s_duelOut, 0x4000 | s_duelBlob);
    native_coop_note("Duel: sent message %d (kind %d)", m->seq, kind);
}

int native_coop_duel_take(int *kind, void *data, int max) {
    DuelMsg in[DUEL_MSGS];
    int i;

    if (native_coop_blob_any(s_pvp.peer, in, sizeof in) != (int)sizeof in) {
        return -1;
    }
    for (i = 0; i < DUEL_MSGS; i++) {
        if (in[i].seq == s_duelTaken + 1) {
            s_duelTaken++;
            int size = SDL_min((int)in[i].size, DUEL_DATA); /* the other's word: never past the message's data */

            *kind = in[i].kind;
            SDL_memcpy(data, in[i].data, SDL_min(max, size));
            native_coop_note("Duel: took message %d (kind %d)", in[i].seq, in[i].kind);
            return size;
        }
    }
    return -1;
}

NativeCoopPvp *native_coop_raid(void) {
    return s_pvp.on && s_pvp.raid ? &s_pvp : NULL;
}

void native_coop_raid_begin(const int *seatPeer, int players, const void *teams, const void *save, int battle,
                            int encounter, int tier) {
    int i;

    SDL_memset(&s_pvp, 0, sizeof s_pvp);
    SDL_memcpy(s_pvp.save, save, sizeof s_pvp.save);
    s_pvp.players = SDL_clamp(players, 1, COOP_RAID_SEATS);
    s_pvp.me = 0;
    for (i = 0; i < s_pvp.players; i++) {
        s_pvp.seatPeer[i] = seatPeer[i];
        if (seatPeer[i] < 0) {
            s_pvp.me = i;
        } else {
            s_pvp.peer = seatPeer[i]; /* one of them, for what reads a single peer */
            native_coop_commands_out(seatPeer[i], s_pvp.out);
        }
        SDL_memcpy(s_pvp.teams[i], (const unsigned char *)teams + i * COOP_TEAM_MAX, COOP_TEAM_MAX);
    }
    s_pvp.battle = battle & 0xFFFF;
    s_pvp.first = s_pvp.me == 0;
    s_pvp.raid = encounter;
    s_pvp.raidTier = tier;
    s_pvp.on = 1;
    g_ff_speed = 1; /* no fast forward in a battle in lockstep (mods.c ff_cycle) */
}

int native_coop_raid_take(int seat, NativeCoopCommand *c) {
    NativeCoopCommand in[COOP_COMMANDS];
    unsigned int now = SDL_GetTicks();
    int peer;
    int n;
    int i;

    if (!s_pvp.on || seat < 0 || seat >= s_pvp.players || (peer = s_pvp.seatPeer[seat]) < 0) {
        return -1;
    }
    if (s_pvp.seatWait[seat] == 0) {
        s_pvp.seatWait[seat] = now;
    }
    n = native_coop_commands_in(peer, in);
    for (i = 0; i < n; i++) {
        if (in[i].battle == (unsigned short)s_pvp.battle && in[i].number == s_pvp.seatTaken[seat] + 1) {
            *c = in[i];
            s_pvp.seatTaken[seat]++;
            s_pvp.seatWait[seat] = 0;
            native_coop_note("Raid: took seat %d's command %d: action %d arg %d digimon %d%s", seat, c->number, c->action,
                             c->arg, c->digimon, c->knockOut ? " (after a knockout)" : "");
            return 1;
        }
    }
    if (now - s_pvp.seatWait[seat] > 8000) {
        NativeCoopState st;

        /* silent, or here but back on the field (a battle sends mode -1); a player still picking its first command has
           none of this battle's out yet, and isn't gone for that */
        if (n == 0 || (native_coop_peer(peer, &st) && st.mode != -1)) {
            native_coop_note("Raid: player %d (seat %d) is gone", peer + 2, seat);
            return -1;
        }
    }
    return 0;
}

int g_coop_raid_reward;

int native_coop_reward_mult(void) {
    return g_coop_raid_reward ? COOP_RAID_REWARD : 1;
}

void native_coop_pvp_begin(int peer, const void *team, int size, const void *save, int battle, int first) {
    SDL_memset(&s_pvp, 0, sizeof s_pvp);
    SDL_memcpy(s_pvp.team, team, SDL_min(size, (int)sizeof s_pvp.team));
    SDL_memcpy(s_pvp.save, save, sizeof s_pvp.save);
    s_pvp.peer = peer;
    s_pvp.battle = battle & 0xFFFF;
    s_pvp.first = first;
    s_pvp.on = 1;
    g_ff_speed = 1; /* no fast forward in a battle in lockstep (mods.c ff_cycle) */
    native_coop_commands_out(peer, s_pvp.out);
}

int native_coop_lockstep(void) {
    return s_pvp.on;
}

void native_coop_pvp_end(void) {
    s_pvp.on = 0;
    SDL_memset(s_pvp.out, 0, sizeof s_pvp.out);
    native_coop_commands_out(s_pvp.peer, s_pvp.out);
    for (int i = 0; s_pvp.raid && i < s_pvp.players; i++) {
        if (s_pvp.seatPeer[i] >= 0) {
            native_coop_commands_out(s_pvp.seatPeer[i], s_pvp.out);
        }
    }
}


void native_coop_pvp_send(NativeCoopCommand *c) {
    c->battle = (unsigned short)s_pvp.battle;
    c->number = (unsigned short)++s_pvp.sent;
    SDL_memmove(&s_pvp.out[1], &s_pvp.out[0], sizeof s_pvp.out[0] * (COOP_COMMANDS - 1));
    s_pvp.out[0] = *c;
    if (s_pvp.raid) {
        for (int i = 0; i < s_pvp.players; i++) {
            if (s_pvp.seatPeer[i] >= 0) {
                native_coop_commands_out(s_pvp.seatPeer[i], s_pvp.out);
            }
        }
    } else {
        native_coop_commands_out(s_pvp.peer, s_pvp.out);
    }
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

unsigned int native_coop_roll_peek(void) {
    return s_roll;
}

unsigned int native_coop_coin(void) {
    Uint64 t = SDL_GetPerformanceCounter();

    return (unsigned int)(t ^ (t >> 32)) * 2654435761u ^ (unsigned int)g_coop_port * 40503u;
}
