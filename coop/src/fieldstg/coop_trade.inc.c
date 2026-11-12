/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): the card trade's state and the raid bosses' list. Not built alone:
   it uses what field.c and the earlier parts define. */

/* --- The card trade, at a Guardromon: talking to one (native_coop_guard_key) offers it before his own talk; who
   chose it waits, and two players waiting (each at any Guardromon) trade. Only spare cards go: the copies none of the
   three decks needs (a card counts once in GAME.cards however many decks use it, so a deck that uses it n times keeps
   n). Each side's state rides on its packets (native_coop_trade_out/in), whole, every frame; a trade is applied when
   both sides have confirmed the same two offers (their hashes), once (its session), and from confirming on neither
   side can go back. Both players stand still meanwhile (FIELDSTG_state.busy) --- */

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#define COOP_CARDS 0x13D /* GAME.cards'; the album's are from 1 */
#define COOP_TRADE_ROWS 5
#define COOP_TRADE_MENU 10 /* coopTrade.phase: the Guardromon's choice, talk or trade */
#define COOP_TRADE_RAID_PICK 11 /* coopTrade.phase: the raid's boss picked from the list */
#define COOP_RAID_ROWS 8
#define COOP_ROOM_WAIT (60 * 10) /* frames the raid's room waits once two are in */
#define COOP_TRADE_WAIT (60 * 60 * 5) /* frames a wait for another player lasts */
#define COOP_TRADE_LINGER 90 /* frames the last word (done, no) keeps going out once the window is shut */
#define COOP_TRADE_SHOW 120 /* frames a message stays */

/* The raids: an encounter, the name of its boss (in the Digimon names' text), its level and tier (0 low, 1 middle,
   2 high: its loot, wfightmn/menu.c). A player sees those up to five levels over its best partner's; the first, open
   to all. Two players pick the same one to fight it together. */
typedef struct CoopRaidBoss {
    s16 encounter;
    s16 nameId;
    s16 level;
    s16 tier;
} CoopRaidBoss;

static const CoopRaidBoss coopRaidBosses[] = {
    {200, 65, 1, 0},   /* Kuwagamon */
    {1, 212, 12, 0},   /* Pharaohmon */
    {2, 213, 14, 0},   /* MasterTyrannomon */
    {323, 219, 15, 0}, /* Bulbmon */
    {264, 232, 17, 0}, /* MetalGreymon */
    {7, 215, 32, 0},   /* HiAndromon */
    {8, 216, 32, 0},   /* WaruMonzaemon */
    {9, 217, 34, 0},   /* Datamon */
    {11, 214, 40, 1},  /* Zanbamon */
    {13, 228, 45, 1},  /* BlackMegaGargomon */
    {15, 229, 46, 1},  /* BlackImperialdramon */
    {17, 227, 47, 1},  /* BlackSeraphimon */
    {31, 226, 53, 1},  /* BlackWarGreymon */
    {23, 230, 55, 1},  /* BlackWarGrowlmon */
    {24, 231, 57, 1},  /* BlackKingNumemon */
    {33, 222, 57, 1},  /* Fujinmon, Suijinmon and Raijinmon */
    {301, 232, 60, 2}, /* MetalGreymon */
    {302, 234, 60, 2}, /* WarGrowlmon */
    {309, 235, 60, 2}, /* MagnaAngemon */
    {303, 233, 65, 2}, /* Paildramon */
    {316, 228, 65, 2}, /* BlackMegaGargomon */
    {317, 229, 65, 2}, /* BlackImperialdramon */
    {310, 237, 70, 2}, /* Kyukimon */
    {318, 227, 70, 2}, /* BlackSeraphimon */
    {319, 230, 70, 2}, /* BlackWarGrowlmon */
    {320, 231, 75, 2}, /* BlackKingNumemon */
    {321, 226, 75, 2}, /* BlackWarGreymon */
    {305, 145, 80, 2}, /* Pharaohmon, MasterTyrannomon and Kimeramon */
    {30, 218, 99, 2},  /* Knightmon */
};
#define COOP_RAID_BOSSES ((s32)(sizeof coopRaidBosses / sizeof coopRaidBosses[0]))

/* the raids this player sees, by their index in coopRaidBosses: how many */
static s32 coopRaidOpen(s32 *list) {
    s32 best = 1;
    s32 n = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (GAME.party[i] >= 0 && GAME.party[i] < PARTNER_COUNT && GAME.partners[GAME.party[i]].info.stats[STAT_LEVEL] > best) {
            best = GAME.partners[GAME.party[i]].info.stats[STAT_LEVEL];
        }
    }
    for (i = 0; i < COOP_RAID_BOSSES; i++) {
        if (i == 0 || coopRaidBosses[i].level <= best + 5) {
            list[n++] = i;
        }
    }
    return n;
}

static struct {
    s32 phase; /* COOP_TRADE_* (this side), or COOP_TRADE_MENU */
    s32 peer;
    u32 session;
    s32 frames; /* in the phase */
    s32 froze; /* busy was set here */
    u32 readyTheirs; /* their offer's hash when this side got ready */
    s32 menu; /* the Guardromon's choice: 0 talk, 1 trade, 2 battle, 3 duel, 4 raid, 5 exit */
    s32 kind; /* what the wait is for: 0 a card trade, 1 a PvP battle, 2 a raid, 3 a card duel (NativeCoopTrade.pad[0]) */
    s32 raid; /* the raid picked: coopRaidBosses' index (NativeCoopTrade.pad[1]) */
    s32 raidCursor, raidTop;
    u32 room; /* the raid's room: its host's session, 0 while gathering */
    s32 roomCount; /* its players, this one among them */
    u32 roomSessions[4]; /* theirs, smallest (the host) first: the seats' order */
    s32 roomPeers[4]; /* each one's peer, -1 this player */
    s32 roomFrames; /* the host's: since two were in */
    s32 teamSent;
    Actor *talker; /* who talked to him, and where: his own talk, if that's the choice */
    Point talkAt;
    s32 passing; /* his own talk going on: not offered again */
    s32 quiet; /* frames after a trade shuts: the press that shut it doesn't talk to him again */
    NativeCoopTrade last; /* the last word, going out a while after */
    s32 linger;
    s32 msg, msgPeer, msgFrames; /* a message after the window: a COOP_TEXT_*, whose name, frames left */
    s32 cursor, top, count; /* the spare cards' list */
    s16 list[COOP_CARDS];
    u8 spare[COOP_CARDS];
    u8 offered[COOP_CARDS];
    s32 note; /* a line under the list: COOP_TEXT_TRADE_NO_ROOM, COOP_TEXT_TRADE_FULL, -1 none */
} coopTrade = {COOP_TRADE_NONE, -1};

/* The spare copies of each card, and the list of the ones there are */
static void coopTradeList(void) {
    u8 use[COOP_CARDS];
    u8 most[COOP_CARDS];
    s32 d;
    s32 i;
    s32 c;

    for (c = 0; c < COOP_CARDS; c++) {
        most[c] = 0;
    }
    for (d = 0; d < DECK_COUNT; d++) {
        for (c = 0; c < COOP_CARDS; c++) {
            use[c] = 0;
        }
        for (i = 0; i < DECK_SIZE; i++) {
            c = GAME.decks[d].cards[i];
            if (c >= 0 && c < COOP_CARDS && ++use[c] > most[c]) {
                most[c] = use[c];
            }
        }
    }
    coopTrade.count = 0;
    for (c = 0; c < COOP_CARDS; c++) {
        if (c == 0) { /* not a card */
            coopTrade.spare[c] = coopTrade.offered[c] = 0;
            continue;
        }
        coopTrade.spare[c] = GAME.cards[c] > most[c] ? GAME.cards[c] - most[c] : 0;
        if (coopTrade.offered[c] > coopTrade.spare[c]) {
            coopTrade.offered[c] = coopTrade.spare[c];
        }
        if (coopTrade.spare[c] != 0) {
            coopTrade.list[coopTrade.count++] = c;
        }
    }
    if (coopTrade.cursor >= coopTrade.count) {
        coopTrade.cursor = coopTrade.count > 0 ? coopTrade.count - 1 : 0;
    }
}

static u32 coopTradeHash(const NativeCoopTrade *t) {
    u32 h = 0x811C9DC5u ^ t->count;
    s32 i;

    for (i = 0; i < t->count && i < COOP_TRADE_KINDS; i++) {
        h = (h ^ t->card[i]) * 0x01000193u;
        h = (h ^ t->copies[i]) * 0x01000193u;
    }
    return h | 1;
}

/* This side's offer, as it goes out */
static void coopTradeOffer(NativeCoopTrade *t) {
    s32 c;

    t->count = 0;
    for (c = 0; c < COOP_CARDS && t->count < COOP_TRADE_KINDS; c++) {
        if (coopTrade.offered[c] != 0) {
            t->card[t->count] = c;
            t->copies[t->count++] = coopTrade.offered[c];
        }
    }
    t->offer = coopTradeHash(t);
}

static s32 coopTradeKinds(void) {
    s32 c;
    s32 n = 0;

    for (c = 0; c < COOP_CARDS; c++) {
        n += coopTrade.offered[c] != 0;
    }
    return n;
}

/* Whether all of their offer fits (9 copies of a card at most) */
static s32 coopTradeRoom(const NativeCoopTrade *in) {
    s32 i;

    if (in->count > COOP_TRADE_KINDS) { /* the other's word: never past the offer's lists */
        return 0;
    }
    for (i = 0; i < in->count; i++) {
        if (in->card[i] == 0 || in->card[i] >= COOP_CARDS || in->copies[i] == 0 ||
            GAME.cards[in->card[i]] + in->copies[i] > CARD_COPIES_MAX) {
            return 0;
        }
    }
    return 1;
}

/* Player n is in a field now (anywhere): its state, else 0 */
static s32 coopTradePeer(s32 n, NativeCoopState *st) {
    return n >= 0 && n < COOP_GHOSTS && native_coop_peer(n, st);
}

static s32 coopTradeFree(void) {
    return FIELDSTG_state.busy == 0 && FIELDSTG_state.acting == 0 && FIELDSTG_state.innOpen == 0 &&
           FIELDSTG_state.battleStarting == 0;
}

static void coopTradeFreeze(void) {
    Actor *player = TASK_REGISTRY.funcs.find(FIELD_TASK_ACTOR, FIELD_CHARACTER_PLAYER, -1);

    if (FIELDSTG_state.busy == 0) {
        FIELDSTG_state.busy = 1;
        coopTrade.froze = 1;
    }
    if (player != NULL && player->key1 == FIELD_CHARACTER_PLAYER) {
        player->setSubstate(player, ACTOR_STAND);
    }
}

/* The window shut: last says what goes out a while longer, msg (a COOP_TEXT_*, -1 none) what this side reads */
static void coopTradeEnd(s32 lastPhase, s32 msg) {
    s32 c;

    coopTrade.last.session = coopTrade.session;
    coopTrade.last.phase = lastPhase;
    coopTrade.last.count = 0;
    coopTrade.linger = lastPhase != COOP_TRADE_NONE ? COOP_TRADE_LINGER : 0;
    coopTrade.msg = msg;
    coopTrade.msgPeer = coopTrade.peer;
    coopTrade.msgFrames = msg >= 0 ? COOP_TRADE_SHOW : 0;
    if (coopTrade.froze) {
        FIELDSTG_state.busy = 0;
        coopTrade.froze = 0;
    }
    coopTrade.quiet = 20;
    coopTrade.phase = COOP_TRADE_NONE;
    for (c = 0; c < COOP_CARDS; c++) {
        coopTrade.offered[c] = 0;
    }
}

static void coopTradeReset(void) {
    s32 c;

    coopTrade.phase = COOP_TRADE_NONE;
    coopTrade.froze = 0; /* the new field isn't busy */
    coopTrade.linger = 0;
    coopTrade.msgFrames = 0;
    for (c = 0; c < COOP_CARDS; c++) {
        coopTrade.offered[c] = 0;
    }
}

static void coopTradeStart(s32 peer, u32 session, s32 phase) {
    coopTrade.peer = peer;
    coopTrade.session = session;
    coopTrade.phase = phase;
    coopTrade.frames = 0;
    coopTrade.cursor = 0;
    coopTrade.top = 0;
    coopTrade.note = -1;
    coopTrade.linger = 0;
    coopTrade.msgFrames = 0;
    coopTradeList();
    coopTradeFreeze();
}

/* The talk button (FIELDSTG_talkToActorAt, target what stands at pos, the tile ahead): a Guardromon, with co-op on,
   first offers the card trade. 1 when this took the talk */
s32 coopTradeTalk(Actor *actor, Point *pos, Actor *target) {
    if (g_coop_debug) {
        native_log("coop: talk at (%d, %d), you at (%d, %d): character %X", pos->x, pos->y, actor->tile.x, actor->tile.y,
                   target != NULL ? target->key1 : -1);
    }
    if (!g_coop || coopTrade.passing || target == NULL || target->key1 != native_coop_guard_key() ||
        native_coop_peer_slots() == 0) {
        return 0;
    }
    if (coopTrade.phase != COOP_TRADE_NONE || coopTrade.quiet > 0 || coopTrade.msgFrames > 0) {
        return 1; /* a trade just shut (its last press, its message): no talk with him yet */
    }
    coopTradeStart(-1, 0, COOP_TRADE_MENU);
    coopTrade.menu = 0;
    coopTrade.talker = actor;
    coopTrade.talkAt = *pos;
    SOUND.playSound(SOUND_MENU_OPEN);
    return 1;
}
