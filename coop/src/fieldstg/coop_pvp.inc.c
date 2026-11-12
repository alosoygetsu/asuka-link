/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): the PvP battle and the card duel. Not built alone:
   it uses what field.c and the earlier parts define. */

/* ---- The PvP battle: once two players waiting at a Guardromon to battle find each other, each sends its party (its
   three partners, whole) and, with both parties across, both start the battle: the other's party as the enemies, the
   save kept to be put back after (wfightmn/menu.c), fightstg/stats.c working out the enemies' stats from their
   partners ---- */

typedef struct CoopTeam { /* fightstg/stats.c has it too */
    s32 party[3];
    s32 ids[3];
    Partner partners[3];
    u32 coin; /* who goes first on a tie of speeds (the smaller): fightstg/stats.c's ends before it */
} CoopTeam;

static CoopTeam coopTeamMine;
static CoopTeam coopTeamTheirs;

/* a party from the network is put into GAME.partners for the battle: 1 if every number in it is one the game can take
   (party, slots, forms, equipment, level), and its mod forms are mods loaded here; else 0, with the reason logged */
static s32 coopTeamValid(const CoopTeam *t) {
    s32 i;
    s32 k;
    s32 any = 0;

    for (i = 0; i < 3; i++) {
        const PartnerStats *p = &t->partners[i].info;

        if (t->party[i] < 0) {
            continue;
        }
        if (t->party[i] >= PARTNER_COUNT || p->stats[STAT_LEVEL] < 1 || p->stats[STAT_LEVEL] > 99 ||
            p->stats[STAT_MAX_HP] < 1) {
            native_coop_note("A party from the network was refused: member %d isn't a partner the game knows", i);
            return 0;
        }
        any = 1;
        /* a slot holds a form's id (setPartnerSlots copies entries[i].id), not its index in entries[], or -1 */
        for (k = 0; k < 4; k++) {
            s32 id = p->slots[k];

            if (id < -1 || id >= NATIVE_MOD_FIRST_ID + 4 ||
                (id >= NATIVE_MOD_FIRST_ID && native_mod_at(id - NATIVE_MOD_FIRST_ID) == NULL)) {
                native_coop_note("A party from the network was refused: slot %d holds form %d, out of range", k, id);
                return 0;
            }
        }
        for (k = 0; k < 6; k++) {
            if (p->equip[k] < 0 || p->equip[k] >= 0x193) {
                native_coop_note("A party from the network was refused: equipment out of range");
                return 0;
            }
        }
        for (k = 0; k < PARTNER_ENTRY_COUNT; k++) {
            s32 id = p->entries[k].id;

            if (id < 0 || id >= NATIVE_MOD_FIRST_ID + 4) {
                native_coop_note("A party from the network was refused: form %d out of range", id);
                return 0;
            }
            if (id >= NATIVE_MOD_FIRST_ID && native_mod_at(id - NATIVE_MOD_FIRST_ID) == NULL) {
                native_coop_note("A party from the network was refused: it has a Digimon mod (form %d) this game "
                                 "doesn't have", id);
                return 0;
            }
        }
    }
    return any;
}
static Battle coopPvpBattle = {1, 15, MUSIC(2, 0)}; /* an encounter of none, the arena's stage and music */

static s32 coopPvpBlob(void) {
    return (s32)((coopTrade.session & 0x7FFF) | 1);
}

static void coopPvpStart(void) {
    s32 i;
    s32 member;
    PartnerStats *st;

    /* a tie of speeds: the smaller coin first (a tie of coins too: the party's bytes, both see the same two) */
    native_coop_pvp_begin(coopTrade.peer, &coopTeamTheirs, sizeof coopTeamTheirs, &GAME, coopPvpBlob(),
                          coopTeamMine.coin != coopTeamTheirs.coin ? coopTeamMine.coin < coopTeamTheirs.coin
                                                                   : memcmp(&coopTeamMine, &coopTeamTheirs, sizeof coopTeamMine) < 0);
    native_coop_note("PvP battle with player %d", coopTrade.peer + 2);
    coopTradeEnd(COOP_TRADE_DONE, -1);
    coopTrade.quiet = 0;
    /* an exhibition: both sides whole (the save comes back after anyway) */
    for (i = 0; i < 3; i++) {
        member = GAME.party[i];
        if (member >= 0 && member < PARTNER_COUNT) {
            st = &GAME.partners[member].info;
            st->stats[STAT_HP] = st->stats[STAT_MAX_HP];
            st->stats[STAT_MP] = st->stats[STAT_MAX_MP];
        }
    }
    FIELDSTG_startBattle(&coopPvpBattle);
    BATTLE_SETUP.ambushChance = 0;
    BATTLE_SETUP.encounterKind = 1;
    BATTLE_SETUP.hasPrize = 0;
    for (i = 0; i < 12; i++) {
        BATTLE_SETUP.blocks[i] = 0;
    }
    for (i = 0; i < 3; i++) {
        member = coopTeamTheirs.party[i];
        BATTLE_SETUP.enemies[i].fighter = 0;
        if (member < 0 || member >= PARTNER_COUNT) {
            continue;
        }
        st = &coopTeamTheirs.partners[i].info;
        BATTLE_SETUP.enemies[i].fighter = coopTeamTheirs.ids[i];
        BATTLE_SETUP.enemies[i].level = st->stats[STAT_LEVEL];
        BATTLE_SETUP.enemies[i].hp = st->stats[STAT_MAX_HP];
        BATTLE_SETUP.enemies[i].mp = st->stats[STAT_MAX_MP];
        BATTLE_SETUP.enemies[i].strength = 16;
    }
}

/* ---- The card duel: two players who picked Duel at a Guardromon go into a card battle each, the other's deck as the
   opponent's (cardgame/battle.c: the decks go across once each has chosen one), an exhibition: the save as it was
   after (cardgame/scene.c) ---- */
static void coopDuelStart(void) {
    native_coop_pvp_begin(coopTrade.peer, &coopTeamMine, 0, &GAME, coopPvpBlob(), 0);
    native_coop_duel_set();
    native_coop_note("Card duel with player %d", coopTrade.peer + 2);
    coopTradeEnd(COOP_TRADE_DONE, -1);
    coopTrade.quiet = 0;
    FIELDSTG_leaveField(MODE_CARD_GAME, 1, 0, 0, 0); /* the first opponent's battle (its level shown): the deck is the other's */
}

/* Nothing to send first: both ready at once, "go" a few frames, then the duel */
static void coopDuelUpdate(const NativeCoopTrade *in) {
    if (PAD_PRESSED(PAD_CIRCLE)) {
        SOUND.playSound(SOUND_MENU_CANCEL);
        coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_OPEN) {
        coopTrade.phase = COOP_TRADE_READY;
    }
    if (coopTrade.phase == COOP_TRADE_READY && in->session == coopTrade.session && in->phase >= COOP_TRADE_READY &&
        in->phase <= COOP_TRADE_DONE) {
        coopTrade.phase = COOP_TRADE_CONFIRM;
        coopTrade.frames = 0;
    }
    if (coopTrade.phase == COOP_TRADE_CONFIRM && coopTrade.frames >= 10) {
        coopDuelStart();
    }
}

/* The parties across (each side READY once it has both), then the battle, when both are */
static void coopPvpUpdate(const NativeCoopTrade *in) {
    s32 i;
    s32 member;

    if (!coopTrade.teamSent) {
        for (i = 0; i < 3; i++) {
            member = GAME.party[i];
            coopTeamMine.party[i] = member;
            coopTeamMine.ids[i] = member >= 0 && member < PARTNER_COUNT ? DIGIMON_DATA[member].id : 0;
            if (i == 0 && member >= 0 && member < PARTNER_COUNT) {
                s16 slots[4];

                /* the first starts the battle as its battle digivolution, as on its own side (wfightmn/menu.c) */
                if (GAME.funcs.getPartnerSlots(member, slots) > 0 && GAME.partners[member].battleDigivolve != 0) {
                    coopTeamMine.ids[0] = GAME.partners[member].battleDigivolve;
                }
            }
            if (member >= 0 && member < PARTNER_COUNT) {
                coopTeamMine.partners[i] = GAME.partners[member];
            }
        }
        coopTeamMine.coin = native_coop_coin();
        native_coop_blob_send(coopTrade.peer, &coopTeamMine, sizeof coopTeamMine, coopPvpBlob());
        coopTrade.teamSent = 1;
    }
    if (PAD_PRESSED(PAD_CIRCLE)) {
        SOUND.playSound(SOUND_MENU_CANCEL);
        coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
        return;
    }
    if (g_coop_debug && coopTrade.frames % 30 == 0) {
        native_log("coop: PvP wait: phase %d session %X; theirs phase %d session %X; blob in %d, ours across %d",
                   coopTrade.phase, coopTrade.session, in->phase, in->session,
                   native_coop_blob_recv(coopTrade.peer, coopPvpBlob(), &coopTeamTheirs, sizeof coopTeamTheirs),
                   native_coop_blob_sent(coopTrade.peer, coopPvpBlob()));
    }
    if (coopTrade.phase == COOP_TRADE_OPEN &&
        native_coop_blob_recv(coopTrade.peer, coopPvpBlob(), &coopTeamTheirs, sizeof coopTeamTheirs) == sizeof coopTeamTheirs &&
        native_coop_blob_sent(coopTrade.peer, coopPvpBlob())) {
        if (!coopTeamValid(&coopTeamTheirs)) {
            coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
            return;
        }
        coopTrade.phase = COOP_TRADE_READY;
    }
    /* both ready: "go" goes out a few frames (the field is left at once after), then the battle */
    if (coopTrade.phase == COOP_TRADE_READY && in->session == coopTrade.session && in->phase >= COOP_TRADE_READY &&
        in->phase <= COOP_TRADE_DONE) {
        coopTrade.phase = COOP_TRADE_CONFIRM;
        coopTrade.frames = 0;
    }
    if (coopTrade.phase == COOP_TRADE_CONFIRM && coopTrade.frames >= 10) {
        coopPvpStart();
    }
}
