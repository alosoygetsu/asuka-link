/* The co-op: the other players (platform/coop.c, coop_look.c, coop_tags.c, coop_battle.c, coop_test.c, coop_net.c;
   patches in coop/patches/, and their parts in coop/src/: fieldstg/coop_*.inc.c, which field.c includes) */
#ifndef NATIVE_COOP_H
#define NATIVE_COOP_H

extern int g_coop;
extern int g_coop_debug;
extern char g_coop_peer[256]; /* coop_peers = a, b, c: the other players, host or host:port (platform/coop_net.c) */
extern int g_coop_port;       /* coop_port: where this copy listens, 27500 */
/* native.ini's coop, coop_peer and coop_port (main.c's read_ini): 1 when key was one of them */
int native_coop_ini(const char *key, const char *value);
int native_player_model(void);
int native_player_tone(void); /* DW3_PLAYER_TONE (tests), -1 none */ /* the player's look: a character (Actor.key1), 0 Junpei's own */
extern int g_exp_mult;   /* exp_multiplier = 1-5 */
extern int g_money_mult; /* money_multiplier = 1-5 */
extern int g_sure_training; /* sure_training = 1: the gyms' tries all work and give their most (coop/patches stgtrain/) */
/* What the field shows of a player: the field's mode and stage file (the same
   place when both match), the actor's pos and z (1/256 units), direction,
   animation set and depth layer */
typedef struct NativeCoopState {
    int mode;
    int stageFile;
    int x, y, z;
    int dir;
    int animSet;
    int depth;
    /* the character creator's look: shown (0 Junior in the player's colour), its model (Actor.key1, 0 Junpei), tone */
    int hasLook, model, tone;
    char name[16]; /* GAME.name, the game's text bytes */
} NativeCoopState;
void native_coop_reset(void);
void native_coop_publish(const NativeCoopState *now);
/* the other player's state now: 1 when there is one */
int native_coop_peer(int i, NativeCoopState *out);
/* how many other players there can be (up to 3): coop_peers', or 1 without it (the ghost of yourself) */
int native_coop_peer_slots(void);
/* a line in coop.log (and the console): what a player can send when something is off */
void native_coop_note(const char *fmt, ...);
/* The card trade (fieldstg/field.c) rides on the field's packets: each packet to a player carries this copy's side of
   its trade with that player, whole, every frame, so nothing needs resending. phase: COOP_TRADE_* */
#define COOP_TRADE_KINDS 5 /* different cards in an offer */
enum { COOP_TRADE_NONE, COOP_TRADE_ASK, COOP_TRADE_OPEN, COOP_TRADE_READY, COOP_TRADE_CONFIRM, COOP_TRADE_DONE,
       COOP_TRADE_NO };
typedef struct NativeCoopTrade {
    unsigned int session; /* the asker's number for this trade, 0 none */
    unsigned char phase, count, pad[2];
    unsigned short card[COOP_TRADE_KINDS];
    unsigned char copies[COOP_TRADE_KINDS];
    unsigned char pad2[1];
    unsigned int offer; /* this side's offer's hash, and the other's as this side sees it */
    unsigned int seen;
} NativeCoopTrade;
void native_coop_tags_block(int on); /* no name tags while on */
int native_coop_test_cards(void); /* DW3_COOP_CARDS (tests) */
int native_coop_auto(void); /* DW3_COOP_AUTO (tests): 0 trade, 1 battle, -1 none */
int native_coop_auto_battle(void); /* DW3_COOP_AUTO=battle: and in the PvP battle, the commands picked by themselves; =giveup (2): the third gives up */
void native_coop_test_save(void *game); /* DW3_COOP_SAVE=card.mcd@0x2300: --field starts with that save (tests) */
int native_test(const char *what);
int native_test_window(void); /* DW3_TEST_WINDOW=1: a test copy, out of the player's way (video.c, pad.c) */ /* DW3_TEST="digilog,...": what a test (scripts/probar.ps1) wants done or dumped */
int native_coop_auto_press(void);
void native_coop_auto_quiet(void); /* no presses now: the test picks the command itself */ /* and cross pressed now and then, for its messages (platform/pad.c) */
int native_coop_guard_key(void); /* the character whose talk offers the trade: Guardromon */
/* Blobs bigger than a packet, to one player, in pieces: id (not 0) names it; sent tells it's all there */
#define COOP_BLOB_MAX 4096
#define COOP_BLOB_PIECE 256
void native_coop_blob_send(int peer, const void *data, int size, int id);
int native_coop_blob_sent(int peer, int id);
int native_coop_blob_recv(int peer, int id, void *out, int max); /* its size when it's whole, else 0 */
/* The PvP battle (fieldstg/field.c starts it, FIGHTSTG and WFIGHTMN read it): the other player's team, and the save
   as it was before, put back after */
#define COOP_PVP_SAVE 0x26C4 /* GAME_SAVE_SIZE (EU) */
/* Its turns: each side's commands, numbered, ride on the packets to the other, the newest few every packet. A command
   is what that side's player turn did, with the seed of its rolls and the fighters as that side had them (its side
   first), which the other takes over */
#define COOP_COMMANDS 4
typedef struct NativeCoopCommand {
    unsigned short battle;   /* the battle's (its session's), so an old battle's commands aren't taken */
    unsigned short number;   /* 1 the side's first command */
    signed char action;      /* the player turn's: 0 attack, 1 give up, 4 a technique, 5 a switch, -1 nothing */
    unsigned char knockOut;  /* a switch after a knockout */
    short arg, digimon;      /* the technique, or the party slot and the Digimon switched in */
    unsigned int seed;       /* native_coop_random's, for the action */
    short hp[2][3], mp[2][3];
    unsigned char active[2], pad[2];
} NativeCoopCommand;
#define COOP_RAID_SEATS 4
/* the co-op's battles: a message's last line stays this many frames, then it goes on by itself (fightstg/message.c) */
#define COOP_MESSAGE_FRAMES 60
#define COOP_TEAM_MAX 3072 /* a CoopTeam's room */
typedef struct NativeCoopPvp {
    int on, peer;
    int battle;               /* NativeCoopCommand.battle */
    int first;                /* 1 when this side goes first on a tie of speeds */
    int sent, taken;          /* commands sent, and the other's taken */
    int over;                 /* 1 the other gave up, 2 the other is gone */
    int raid;                 /* a raid: the encounter the players fight side by side, all their parties on this side */
    int raidTier;             /* its boss's tier: 0 low, 1 middle, 2 high (its loot) */
    int duel;                 /* a card duel (cardgame/battle.c): the other's deck as the opponent's */
    /* the raid's seats, in the order of the players' room numbers: each one's peer (-1 this player), its commands
       taken, the wait for its next, and its party (fieldstg/field.c's CoopTeam) */
    int players, me;
    int seatPeer[COOP_RAID_SEATS], seatTaken[COOP_RAID_SEATS];
    unsigned int seatWait[COOP_RAID_SEATS];
    unsigned char teams[COOP_RAID_SEATS][COOP_TEAM_MAX];
    unsigned char names[COOP_RAID_SEATS][16]; /* the seats' players' names (the game's text), from the field */
    unsigned int waitFrom;    /* SDL ms the wait for the other's command began, 0 none */
    NativeCoopCommand out[COOP_COMMANDS]; /* the newest sent first */
    unsigned char team[COOP_BLOB_MAX]; /* fieldstg/field.c's CoopTeam */
    unsigned char save[COOP_PVP_SAVE];
} NativeCoopPvp;
NativeCoopPvp *native_coop_pvp(void); /* the battle's, or NULL when it's not one (or a raid) */
/* The raid: the PvP's pairing and commands, both players on this side against a boss, turn about (wfightmn/menu.c) */
NativeCoopPvp *native_coop_raid(void); /* the raid's, or NULL */
NativeCoopPvp *native_coop_duel(void); /* the card duel's, or NULL */
int native_coop_lockstep(void); /* a PvP battle, a raid or a card duel is on: no fast forward (platform/mods.c) */
void native_coop_duel_set(void);
/* the duel's messages (cardgame/): sent in order, the other's taken in order (its size; -1 none yet) */
void native_coop_duel_post(int kind, const void *data, int size);
int native_coop_duel_take(int *kind, void *data, int max);
int native_coop_blob_any(int peer, void *out, int max); /* the other's blob as it is, whole: its size, else 0 */
/* the raid starts: its seats' peers (-1 this player's seat), their parties (players of COOP_TEAM_MAX bytes) */
void native_coop_raid_begin(const int *seatPeer, int players, const void *teams, const void *save, int battle,
                            int encounter, int tier);
/* the next command of a seat's player: 1 taken, 0 not here yet, -1 the player is gone */
int native_coop_raid_take(int seat, NativeCoopCommand *c);
/* a raid won: its report gives COOP_RAID_REWARD times the EXP and money (stfgtrep/report.c, partner.c); 0 again at the
   next battle */
#define COOP_RAID_REWARD 3
extern int g_coop_raid_reward;
int native_coop_reward_mult(void); /* COOP_RAID_REWARD after a raid won, else 1 */
int native_coop_start_delay(void); /* DW3_COOP_START_DELAY=<frames> (tests): the raid starts that much later here */
int native_coop_raid_test(void); /* DW3_COOP_RAID=<encounter> (tests): the Guardromon's battle is that raid; 0 none */
int native_coop_test_tech(int n); /* DW3_COOP_TECH=id,id,... (tests): the auto battle's n-th technique, 0 none */
/* DW3_COOP_<name>=<number> (tests), -1 unset: ITEM=id the raid's auto battle uses that item at its second command,
   FLEE=seat that seat's player runs away from the raid at its third (fightstg/player_turn.c) */
int native_coop_test_env(const char *name);
/* DW3_COOP_EQUIP=slot:item,... and DW3_COOP_HP=n (tests): on each partner sent to a battle */
void native_coop_test_partner(short *stats, short *equip);
void native_coop_pvp_begin(int peer, const void *team, int size, const void *save, int battle, int first);
void native_coop_pvp_end(void);
unsigned int native_coop_coin(void); /* a number of this copy's own, for who goes first */
void native_coop_pvp_pump(void); /* each battle frame: the packets go on (as off the field) */
void native_coop_pvp_send(NativeCoopCommand *c); /* numbers it and sends it */
/* the other's next command: 1 taken, 0 not here yet, -1 the other is gone */
int native_coop_pvp_take(NativeCoopCommand *c);
/* In a PvP battle the rolls (FIGHTSTG's damage, hits, statuses, delays) are the battle's own, seeded by each command,
   so both games roll the same: those files have #define RANDOM (*native_coop_random(&RANDOM)) */
struct Random;
struct Random *native_coop_random(struct Random *game);
void native_coop_pvp_seed(unsigned int seed);
unsigned int native_coop_roll_peek(void); /* the battle's roll as it stands (DW3_COOP_DEBUG's sync lines) */
/* the other's commands as they came (coop_net.c) */
int native_coop_commands_in(int peer, NativeCoopCommand *out); /* how many (COOP_COMMANDS), 0 when it's gone */
void native_coop_commands_out(int peer, const NativeCoopCommand *c);
void native_coop_trade_out(int peer, const NativeCoopTrade *t); /* what goes to peer from now on */
int native_coop_trade_in(int peer, NativeCoopTrade *t); /* peer's side as it last came: 0 when it's gone */
/* the name tags over the other players: where ghost n's head was drawn this frame (screen, 320x240) */
void native_coop_tag(int n, int x, int y);
int native_coop_tags_due(void); /* 1 while a tag was drawn just now: video.c then draws the overlay */
struct SDL_Renderer;
void native_coop_draw_tags(struct SDL_Renderer *r);
/* a model a player may have (the character creator's): 1 for Junpei (0) and the curated ones */
int native_coop_model_ok(int model);
/* VRAM for the field's actors: the slots kept, cleared when the field
   starts, a box kept for one, and a blank box free in a texture page */
void native_coop_vram_reset(void);
void native_coop_vram_reserve(int x, int y, int w, int h);
/* the other players' boxes: one taken (mine), and the last map's blanked again when the new one left them (reclaim) */
void native_coop_vram_mine(int x, int y, int w, int h);
void native_coop_vram_reclaim(void);
int native_coop_find_vram(int page_x, int page_y, int w, int h, int *out_x, int *out_y);
void native_coop_vram_dump(int page_x, int page_y);
/* or in any page right of the frame buffers, and a copy of VRAM (a CLUT) */
int native_coop_find_vram_any(int w, int h, int *out_x, int *out_y, int *page_x, int *page_y);
void native_coop_vram_copy(int from_x, int from_y, int to_x, int to_y, int w, int h);
/* the CLUT at (x, y) recoloured for other player n (0-2) */
void native_coop_vram_tint(int x, int y, int n);
/* the game's menu CLUTs (x, y, w x h) copied to (to_x, to_y) with their dark blues lilac: the co-op's windows */
void native_coop_vram_lilac(int x, int y, int to_x, int to_y, int w, int h);
/* a box's words as one number (has something written over it?), and a box blanked (tests) */
unsigned native_coop_vram_hash(int x, int y, int w, int h);
void native_coop_vram_clear(int x, int y, int w, int h);
void native_coop_vram_forget(int x, int y); /* a box of ours the game wrote over: not ours any more */
int native_coop_test_stomp(void); /* DW3_COOP_STOMP=<frames> (tests): 1 once, that long after the first call */
/* the character creator's tones: src's 16 colours turned for tone (0 as drawn, 1-7) into dst; and a CLUT into VRAM */
#define NATIVE_TONES 8
void native_tone_clut(unsigned short *dst, const unsigned short *src, int tone);
void native_vram_clut(int x, int y, const unsigned short *colors);

/* The free team (platform/team.c): GAME.partySet of a picked team, NATIVE_TEAM_BASE + a + b * 8 + c * 64 */
#define NATIVE_TEAM_BASE 0x40
int native_team_partner(int set, int i); /* partner i of a set past the game's three packs, else -1 */
int native_team_set(const int team[3]);
int native_team_look(int partner); /* the lobby: its look (FIELDSTG_fileEntries) */
void native_team_greeting(int partner, int *sub, int *line); /* and its greeting: sub 0xFF the port's own text */
const void *native_coop_text(int language); /* tools/coop_text.py's texts, as a text file of the language */
#include "coop_text_ids.h" /* their indices, by block and name (COOP_TEXT_TRADE_TITLE...) */

#endif /* NATIVE_COOP_H */
