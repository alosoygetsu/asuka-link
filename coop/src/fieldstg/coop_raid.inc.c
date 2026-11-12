/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): the raid's room. Not built alone:
   it uses what field.c and the earlier parts define. */

/* ---- The raid's room: the players who picked the same raid at a Guardromon gather; the one of smallest session hosts
   it, its list of them (up to 4, by session) going out with its wait. Once two are in, ten seconds more (or four in),
   the host opens it, and those on its list follow: each sends its party to each other, all ready, all start (seats
   by the list). A raid packet: session its sender's own, offer the room (0 gathering), count and the list (in card and
   copies' bytes) the host's, seen its countdown. ---- */
static void coopRoomList(const NativeCoopTrade *t, u32 *out) {
    memcpy(out, t->card, 4 * sizeof(u32));
}

static s32 coopRoomHas(const NativeCoopTrade *t, u32 session) {
    u32 list[4];
    s32 i;

    coopRoomList(t, list);
    for (i = 0; i < t->count && i < 4; i++) {
        if (list[i] == session) {
            return 1;
        }
    }
    return 0;
}

/* the raid packet this player sends */
static void coopRoomPacket(NativeCoopTrade *t) {
    memset(t, 0, sizeof *t);
    t->session = coopTrade.session;
    t->phase = coopTrade.phase;
    t->pad[0] = 2;
    t->pad[1] = coopTrade.raid;
    t->count = coopTrade.roomCount;
    memcpy(t->card, coopTrade.roomSessions, 4 * sizeof(u32));
    t->offer = coopTrade.room;
    t->seen = coopTrade.roomFrames < COOP_ROOM_WAIT ? (COOP_ROOM_WAIT - coopTrade.roomFrames + 59) / 60 : 0;
}

/* a peer's raid packet for this raid: 1 */
static s32 coopRoomPeer(s32 i, NativeCoopTrade *t) {
    return native_coop_trade_in(i, t) && t->session != 0 && t->pad[0] == 2 && t->pad[1] == coopTrade.raid &&
           t->phase >= COOP_TRADE_ASK && t->phase <= COOP_TRADE_CONFIRM;
}

static void coopTeamBuild(CoopTeam *team) {
    s32 i;
    s32 member;

    memset(team, 0, sizeof *team);
    for (i = 0; i < 3; i++) {
        member = GAME.party[i];
        team->party[i] = member;
        team->ids[i] = member >= 0 && member < PARTNER_COUNT ? DIGIMON_DATA[member].id : 0;
        if (i == 0 && member >= 0 && member < PARTNER_COUNT) {
            s16 slots[4];

            /* the first starts the battle as its battle digivolution, as on its own side (wfightmn/menu.c) */
            if (GAME.funcs.getPartnerSlots(member, slots) > 0 && GAME.partners[member].battleDigivolve != 0) {
                team->ids[0] = GAME.partners[member].battleDigivolve;
            }
        }
        if (member >= 0 && member < PARTNER_COUNT) {
            team->partners[i] = GAME.partners[member];
        }
    }
}

static u8 coopRoomTeams[4][COOP_TEAM_MAX];
/* the room's last word once this player started: "go" (CONFIRM) to everyone, the field's last packet, which the
   battle's packets carry on, so that the others still find this player in the room as they start */
static NativeCoopTrade coopRaidGoPacket;

static s32 coopRoomBlob(void) {
    return (s32)((coopTrade.room & 0x7FFF) | 1);
}

/* the raid starts: the seats by the room's list, every party whole, the boss's HP times the players */
static void coopRaidStart(void) {
    const CoopRaidBoss *boss = &coopRaidBosses[coopTrade.raid];
    s32 enc = native_coop_raid_test() > 0 ? native_coop_raid_test() : boss->encounter;
    PartnerStats *st;
    s32 i;
    s32 hp;
    s32 member;

    /* every party whole (the save comes back after a loss anyway) */
    for (i = 0; i < 3; i++) {
        member = GAME.party[i];
        if (member >= 0 && member < PARTNER_COUNT) {
            st = &GAME.partners[member].info;
            st->stats[STAT_HP] = st->stats[STAT_MAX_HP];
            st->stats[STAT_MP] = st->stats[STAT_MAX_MP];
        }
    }
    native_coop_raid_begin(coopTrade.roomPeers, coopTrade.roomCount, coopRoomTeams, &GAME, coopRoomBlob(), enc,
                           native_coop_raid_test() > 0 ? 0 : boss->tier);
    for (i = 0; i < coopTrade.roomCount; i++) {
        /* the players' names, while they're on the field (wfightmn/menu.c's lines) */
        NativeCoopState st;
        u8 *name = native_coop_raid()->names[i];
        s32 k;

        name[0] = 0;
        if (coopTrade.roomPeers[i] < 0) {
            for (k = 0; k < 15 && GAME.name[k] != 0; k++) {
                name[k] = GAME.name[k];
            }
            name[k] = 0;
        } else if (native_coop_peer(coopTrade.roomPeers[i], &st)) {
            for (k = 0; k < 15 && st.name[k] != 0; k++) {
                name[k] = st.name[k];
            }
            name[k] = 0;
        }
    }
    coopRoomPacket(&coopRaidGoPacket);
    coopRaidGoPacket.phase = COOP_TRADE_CONFIRM;
    coopRaidGo = 1;
    native_coop_note("Raid with %d players: encounter %d", coopTrade.roomCount, enc);
    coopTradeEnd(COOP_TRADE_DONE, -1);
    coopTrade.quiet = 0;
    FIELDSTG_startBattle(&coopPvpBattle);
    BATTLE_SETUP.battle = enc;
    BATTLE_SETUP.ambushChance = 0;
    BATTLE_SETUP.encounterKind = FIELDSTG_encounters[enc].kind;
    BATTLE_SETUP.hasPrize = 0;
    for (i = 0; i < 12; i++) {
        BATTLE_SETUP.blocks[i] = FIELDSTG_encounters[enc].blocks[i];
    }
    for (i = 0; i < 3; i++) {
        BATTLE_SETUP.enemies[i] = *FIELDSTG_encounters[enc].enemies[i];
        hp = BATTLE_SETUP.enemies[i].hp * coopTrade.roomCount;
        BATTLE_SETUP.enemies[i].hp = hp > 0x7FFF ? 0x7FFF : hp;
    }
}

/* every frame of the room, from the wait at the Guardromon to the start */
static void coopRaidLobby(void) {
    NativeCoopTrade t;
    NativeCoopTrade host;
    u32 list[4];
    s32 i;
    s32 k;
    s32 n;
    s32 all;

    if (PAD_PRESSED(PAD_CIRCLE)) {
        SOUND.playSound(SOUND_MENU_CANCEL);
        coopTradeEnd(coopTrade.phase == COOP_TRADE_ASK ? COOP_TRADE_NONE : COOP_TRADE_NO,
                     coopTrade.phase == COOP_TRADE_ASK ? -1 : COOP_TEXT_TRADE_CANCELLED);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK) {
        /* a room opened that has this player: in */
        for (i = 0; i < native_coop_peer_slots() && i < COOP_GHOSTS; i++) {
            if (coopRoomPeer(i, &t) && t.phase >= COOP_TRADE_OPEN && t.offer == t.session && coopRoomHas(&t, coopTrade.session)) {
                coopTrade.room = t.offer;
                coopTrade.roomCount = t.count;
                coopRoomList(&t, coopTrade.roomSessions);
                coopTrade.phase = COOP_TRADE_OPEN;
                coopTrade.frames = 0;
                coopTrade.teamSent = 0;
                SOUND.playSound(SOUND_MENU_OPEN);
                return;
            }
        }
        /* the ones waiting for the same raid: this player hosts when its session is the smallest */
        n = 1;
        list[0] = coopTrade.session;
        host.session = 0;
        for (i = 0; i < native_coop_peer_slots() && i < COOP_GHOSTS; i++) {
            if (coopRoomPeer(i, &t) && t.phase == COOP_TRADE_ASK && t.offer == 0) {
                if (n < 4) {
                    list[n++] = t.session;
                }
                if (t.session < coopTrade.session && (host.session == 0 || t.session < host.session)) {
                    host = t;
                }
            }
        }
        if (host.session != 0) {
            /* another hosts: its list and countdown, shown */
            coopTrade.roomCount = host.count;
            coopTrade.roomFrames = COOP_ROOM_WAIT - host.seen * 60;
            return;
        }
        for (i = 1; i < n; i++) { /* the list by session */
            for (k = i; k > 0 && list[k] < list[k - 1]; k--) {
                u32 x = list[k];

                list[k] = list[k - 1];
                list[k - 1] = x;
            }
        }
        coopTrade.roomCount = n;
        memset(coopTrade.roomSessions, 0, sizeof coopTrade.roomSessions);
        memcpy(coopTrade.roomSessions, list, n * sizeof(u32));
        coopTrade.roomFrames = n >= 2 ? coopTrade.roomFrames + 1 : 0;
        if (n >= 2 && (coopTrade.roomFrames >= COOP_ROOM_WAIT || n == 4)) {
            coopTrade.room = coopTrade.session; /* open: the list stays */
            coopTrade.phase = COOP_TRADE_OPEN;
            coopTrade.frames = 0;
            coopTrade.teamSent = 0;
            SOUND.playSound(SOUND_MENU_OPEN);
        }
        return;
    }
    /* the room's players' peers, by their sessions: found while the parties go across, then kept (one that starts
       first may stop saying it's in the room just as the others start) */
    all = 1;
    for (k = 0; k < coopTrade.roomCount && coopTrade.phase == COOP_TRADE_OPEN; k++) {
        if (coopTrade.roomSessions[k] == coopTrade.session) {
            coopTrade.roomPeers[k] = -1;
            continue;
        }
        coopTrade.roomPeers[k] = -2;
        for (i = 0; i < native_coop_peer_slots() && i < COOP_GHOSTS; i++) {
            if (coopRoomPeer(i, &t) && t.session == coopTrade.roomSessions[k]) {
                coopTrade.roomPeers[k] = i;
                if (t.phase < COOP_TRADE_OPEN || t.offer != coopTrade.room || t.phase < coopTrade.phase - 1) {
                    all = 0; /* not in yet, or a step behind */
                }
            }
        }
        if (coopTrade.roomPeers[k] == -2) {
            all = 0;
        }
    }
    if (coopTrade.frames > 60 * 15) {
        native_coop_note("Raid: the room didn't fill (%d players listed)", coopTrade.roomCount);
        coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_OPEN) {
        /* this player's party to each, theirs in */
        if (!coopTrade.teamSent && all) {
            coopTeamBuild(&coopTeamMine);
            for (k = 0; k < coopTrade.roomCount; k++) {
                if (coopTrade.roomPeers[k] >= 0) {
                    native_coop_blob_send(coopTrade.roomPeers[k], &coopTeamMine, sizeof coopTeamMine, coopRoomBlob());
                }
            }
            coopTrade.teamSent = 1;
        }
        n = coopTrade.teamSent;
        for (k = 0; k < coopTrade.roomCount && n; k++) {
            if (coopTrade.roomPeers[k] == -1) {
                memset(coopRoomTeams[k], 0, COOP_TEAM_MAX);
                memcpy(coopRoomTeams[k], &coopTeamMine, sizeof coopTeamMine);
            } else if (coopTrade.roomPeers[k] < 0 ||
                       native_coop_blob_recv(coopTrade.roomPeers[k], coopRoomBlob(), coopRoomTeams[k], COOP_TEAM_MAX) != sizeof coopTeamMine ||
                       !native_coop_blob_sent(coopTrade.roomPeers[k], coopRoomBlob())) {
                n = 0;
            }
        }
        for (k = 0; k < coopTrade.roomCount && n; k++) {
            if (!coopTeamValid((const CoopTeam *)coopRoomTeams[k])) {
                coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
                return;
            }
        }
        if (n) {
            coopTrade.phase = COOP_TRADE_READY;
        }
        return;
    }
    if (coopTrade.phase == COOP_TRADE_READY) {
        /* all ready: "go" a few frames, then the battle */
        for (k = 0; k < coopTrade.roomCount; k++) {
            if (coopTrade.roomPeers[k] >= 0 && (!coopRoomPeer(coopTrade.roomPeers[k], &t) || t.phase < COOP_TRADE_READY)) {
                return;
            }
        }
        coopTrade.phase = COOP_TRADE_CONFIRM;
        coopTrade.frames = 0;
        return;
    }
    if (coopTrade.phase == COOP_TRADE_CONFIRM && coopTrade.frames >= 10 + native_coop_start_delay()) {
        for (k = 0, n = 0; k < coopTrade.roomCount; k++) {
            n += coopTrade.roomPeers[k] == -1 ? 1 : coopTrade.roomPeers[k] < 0 ? 10 : 0;
        }
        if (n != 1) { /* this player once, every other one found */
            native_coop_note("Raid: the room's players weren't all found (%d)", n);
            coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
            return;
        }
        coopRaidStart();
    }
}

/* Every frame of the field with co-op on */
