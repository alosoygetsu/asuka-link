/* The co-op: the other player on the field (platform/coop.c, coop_net.c; patches in coop/patches/) */
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
typedef struct NativeCoopPvp {
    int on, peer;
    unsigned char team[COOP_BLOB_MAX]; /* fieldstg/field.c's CoopTeam */
    unsigned char save[COOP_PVP_SAVE];
} NativeCoopPvp;
NativeCoopPvp *native_coop_pvp(void); /* the battle's, or NULL when it's not one */
void native_coop_pvp_begin(int peer, const void *team, int size, const void *save);
void native_coop_pvp_end(void);
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
int native_coop_find_vram(int page_x, int page_y, int w, int h, int *out_x, int *out_y);
void native_coop_vram_dump(int page_x, int page_y);
/* or in any page right of the frame buffers, and a copy of VRAM (a CLUT) */
int native_coop_find_vram_any(int w, int h, int *out_x, int *out_y, int *page_x, int *page_y);
void native_coop_vram_copy(int from_x, int from_y, int to_x, int to_y, int w, int h);
/* the CLUT at (x, y) recoloured for other player n (0-2) */
void native_coop_vram_tint(int x, int y, int n);
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
#define COOP_TEXT_TITLE 1
#define COOP_TEXT_HELP 2
#define COOP_TEXT_TEAM 3
#define COOP_TEXT_INTRO 4
#define COOP_TEXT_MARK 5 /* 5-7: "1" "2" "3" */
#define COOP_TEXT_SEPARATOR 8
#define COOP_TEXT_NO_TEAM 9
#define COOP_TEXT_ACCEPT 10
#define COOP_TEXT_LOOK_TITLE 11
#define COOP_TEXT_LOOK_HELP 12
#define COOP_TEXT_LOOK_NAME 13 /* 13-15: Junior, the boy, the girl */
#define COOP_TEXT_TRADE 16 /* 16-41 (40-41 the PvP battle's): the card trade's (fieldstg/field.c), in tools/coop_text.py's order */

#endif /* NATIVE_COOP_H */
