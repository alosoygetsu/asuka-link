/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): what the Guardromon's menu does each frame, and the co-op's frame on the field. Not built alone:
   it uses what field.c and the earlier parts define. */

static void coopTradeUpdate(void) {
    NativeCoopTrade in;
    NativeCoopTrade out;
    NativeCoopState st;
    u32 theirs;
    s32 here;
    s32 n;
    s32 c;
    s32 i;

    out.session = 0;
    out.phase = COOP_TRADE_NONE;
    out.count = 0;
    out.offer = out.seen = 0;
    in.session = 0;
    in.phase = COOP_TRADE_NONE;
    in.count = 0;
    in.offer = in.seen = 0;
    coopTrade.frames++;
    if (coopTrade.quiet > 0) {
        coopTrade.quiet--;
    }
    if (coopTrade.msgFrames > 0) {
        coopTrade.msgFrames--;
    }
    if (coopTrade.phase == COOP_TRADE_NONE && coopTrade.frames > 200 && coopTradeFree() && (i = native_coop_auto()) >= 0) {
        coopTradeStart(-1, 0, i == 3 ? COOP_TRADE_MENU : COOP_TRADE_ASK); /* tests: a Guardromon's choice made, or
                                                                              its menu (DW3_COOP_AUTO=menu) open */
        coopTrade.kind = i == 4 ? 3 : i; /* DW3_COOP_AUTO=duel: 4, the duel's kind 3 */
        coopTrade.menu = 0;
        coopTrade.raid = 0;
        coopTrade.room = 0;
        coopTrade.roomCount = 1;
        coopTrade.roomFrames = 0;
        coopTrade.session = (((u32)GFX.funcs.getTime() * 2654435761u) ^ ((u32)GAME.playFrames << 7) ^ native_coop_coin()) | 1;
    }
    n = native_coop_test_cards();
    for (c = 1; c <= 5 && n > 0; c++) {
        GAME.funcs.addCards(c, n);
    }

    if (coopTrade.phase != COOP_TRADE_NONE) {
        here = coopTrade.peer >= 0 && native_coop_trade_in(coopTrade.peer, &in) && coopTradePeer(coopTrade.peer, &st);
        theirs = coopTradeHash(&in);
        if (coopTrade.kind == 2 && coopTrade.phase >= COOP_TRADE_ASK && coopTrade.phase <= COOP_TRADE_CONFIRM) {
            coopRaidLobby();
        } else
        switch (coopTrade.phase) {
        case COOP_TRADE_MENU: {
            /* as the game's list menus: the cursor stops at the ends, held buttons repeat, triangle shuts it */
            s32 rows[COOP_GUARD_ROWS];
            s32 count = coopGuardOptions(rows);
            s32 prev = coopTrade.menu;

            if (!coopGuardListReady(COOP_LIST_MENU)) {
                break; /* its window still opening: the game's menus take no button yet */
            }
            if (PAD_PRESSED(PAD_UP) || PAD_REPEATED(PAD_UP)) {
                coopTrade.menu = coopTrade.menu > 0 ? coopTrade.menu - 1 : 0;
            } else if (PAD_PRESSED(PAD_DOWN) || PAD_REPEATED(PAD_DOWN)) {
                coopTrade.menu = coopTrade.menu < count - 1 ? coopTrade.menu + 1 : count - 1;
            }
            if (coopTrade.menu >= count) {
                coopTrade.menu = count - 1;
            }
            c = rows[coopTrade.menu];
            if (prev != coopTrade.menu) {
                SOUND.playSound(SOUND_CURSOR);
            } else if (PAD_PRESSED(PAD_TRIANGLE) || (PAD_PRESSED(PAD_CROSS) && c == COOP_GUARD_EXIT)) {
                /* exit: the menu shut, no talk */
                SOUND.playSound(PAD_PRESSED(PAD_TRIANGLE) ? SOUND_MENU_CANCEL : SOUND_SELECT);
                coopTradeEnd(COOP_TRADE_NONE, -1);
            } else if (PAD_PRESSED(PAD_CIRCLE) || (PAD_PRESSED(PAD_CROSS) && c == COOP_GUARD_TALK)) {
                /* his own talk (saving), as if this weren't here */
                coopTradeEnd(COOP_TRADE_NONE, -1);
                coopTrade.passing = 1;
                FIELDSTG_talkToActorAt(coopTrade.talker, &coopTrade.talkAt);
                coopTrade.passing = 0;
            } else if (PAD_PRESSED(PAD_CROSS) && c == COOP_GUARD_RAID) {
                coopTrade.phase = COOP_TRADE_RAID_PICK; /* the raid's boss first */
                coopTrade.raidCursor = coopTrade.raidTop = 0;
                SOUND.playSound(SOUND_SELECT);
            } else if (PAD_PRESSED(PAD_CROSS)) {
                coopTrade.kind = c == COOP_GUARD_DUEL ? 3 : c - 1; /* 0 trade, 1 battle, 3 duel */
                coopTrade.teamSent = 0;
                coopTrade.phase = COOP_TRADE_ASK; /* wait for another player at a Guardromon */
                coopTrade.session = (((u32)GFX.funcs.getTime() * 2654435761u) ^ ((u32)GAME.playFrames << 7) ^ native_coop_coin()) | 1;
                coopTrade.frames = 0;
                SOUND.playSound(SOUND_SELECT);
            }
            break;
        }
        case COOP_TRADE_RAID_PICK: {
            s32 open[COOP_RAID_BOSSES];
            s32 count = coopRaidOpen(open);
            s32 prev = coopTrade.raidCursor;

            if (!coopGuardListReady(COOP_LIST_RAIDS)) {
                break; /* its window still opening (or the menu's still shutting) */
            }
            /* as the game's list menus: the cursor stops at the ends, held buttons repeat */
            if (PAD_PRESSED(PAD_UP) || PAD_REPEATED(PAD_UP)) {
                coopTrade.raidCursor = coopTrade.raidCursor > 0 ? coopTrade.raidCursor - 1 : 0;
            } else if (PAD_PRESSED(PAD_DOWN) || PAD_REPEATED(PAD_DOWN)) {
                coopTrade.raidCursor = coopTrade.raidCursor < count - 1 ? coopTrade.raidCursor + 1 : count - 1;
            }
            if (prev != coopTrade.raidCursor) {
                SOUND.playSound(SOUND_CURSOR);
            } else if (PAD_PRESSED(PAD_CIRCLE) || PAD_PRESSED(PAD_TRIANGLE)) {
                coopTrade.phase = COOP_TRADE_MENU; /* back to the Guardromon's choice */
                SOUND.playSound(SOUND_MENU_CANCEL);
            } else if (PAD_PRESSED(PAD_CROSS)) {
                coopTrade.kind = 2;
                coopTrade.raid = open[coopTrade.raidCursor];
                coopTrade.teamSent = 0;
                coopTrade.room = 0;
                coopTrade.roomCount = 1;
                coopTrade.roomFrames = 0;
                coopTrade.phase = COOP_TRADE_ASK; /* wait for another player who picked the same raid */
                coopTrade.session = (((u32)GFX.funcs.getTime() * 2654435761u) ^ ((u32)GAME.playFrames << 7) ^ native_coop_coin()) | 1;
                coopTrade.frames = 0;
                SOUND.playSound(SOUND_SELECT);
            }
            if (coopTrade.raidCursor < coopTrade.raidTop) {
                coopTrade.raidTop = coopTrade.raidCursor;
            }
            if (coopTrade.raidCursor >= coopTrade.raidTop + COOP_RAID_ROWS) {
                coopTrade.raidTop = coopTrade.raidCursor - COOP_RAID_ROWS + 1;
            }
            break;
        }
        case COOP_TRADE_ASK:
            /* another player waiting too: the one of smallest session, and that session for both. The one whose
               session is the larger opens with the other's; the other waits to see it open with its own (were both to
               open, the smaller would open with its own, which the larger never takes for its) */
            n = -1;
            for (i = 0; i < native_coop_peer_slots() && i < COOP_GHOSTS; i++) {
                NativeCoopTrade other;

                if (native_coop_trade_in(i, &other) && other.session != 0 &&
                    other.pad[0] == coopTrade.kind && (coopTrade.kind != 2 || other.pad[1] == coopTrade.raid) &&
                    ((other.phase == COOP_TRADE_ASK && other.session < coopTrade.session) ||
                     (other.phase == COOP_TRADE_OPEN && other.session == coopTrade.session)) &&
                    (n < 0 || other.session < in.session)) {
                    n = i;
                    in = other;
                }
            }
            if (n >= 0) {
                coopTrade.peer = n;
                if (in.session < coopTrade.session) {
                    coopTrade.session = in.session;
                }
                coopTrade.phase = COOP_TRADE_OPEN;
                coopTrade.frames = 0;
                coopTradeList();
                SOUND.playSound(SOUND_MENU_OPEN);
            } else if (coopTrade.frames > COOP_TRADE_WAIT || PAD_PRESSED(PAD_CIRCLE)) {
                SOUND.playSound(SOUND_MENU_CANCEL);
                coopTradeEnd(COOP_TRADE_NONE, -1);
            }
            break;
        default: /* open, ready, confirmed */
            if (g_coop_debug && (!here || in.phase == COOP_TRADE_NO || (in.phase != COOP_TRADE_ASK && in.session != coopTrade.session))) {
                native_log("coop: trade ends: here %d, theirs phase %d session %X, ours %X", here, in.phase, in.session,
                           coopTrade.session);
            }
            if (!here || in.phase == COOP_TRADE_NO ||
                (in.phase == COOP_TRADE_ASK ? coopTrade.frames > 60 : in.session != coopTrade.session) ||
                (in.phase == COOP_TRADE_NONE && coopTrade.frames > 30)) {
                coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
                break;
            }
            if (coopTrade.kind == 3) {
                coopDuelUpdate(&in); /* the card duel's: both ready, then both start */
                break;
            }
            if (coopTrade.kind >= 1) {
                coopPvpUpdate(&in); /* the battle's: the parties across, then both start */
                break;
            }
            if (coopTrade.phase == COOP_TRADE_OPEN) {
                coopTradeList();
                if (coopTrade.count > 0 && (PAD_PRESSED(PAD_UP) || PAD_REPEATED(PAD_UP))) {
                    coopTrade.cursor = (coopTrade.cursor + coopTrade.count - 1) % coopTrade.count;
                    SOUND.playSound(SOUND_CURSOR);
                } else if (coopTrade.count > 0 && (PAD_PRESSED(PAD_DOWN) || PAD_REPEATED(PAD_DOWN))) {
                    coopTrade.cursor = (coopTrade.cursor + 1) % coopTrade.count;
                    SOUND.playSound(SOUND_CURSOR);
                } else if (coopTrade.count > 0 && (PAD_PRESSED(PAD_RIGHT) || PAD_REPEATED(PAD_RIGHT))) {
                    c = coopTrade.list[coopTrade.cursor];
                    if (coopTrade.offered[c] == 0 && coopTradeKinds() >= COOP_TRADE_KINDS) {
                        coopTrade.note = COOP_TEXT_TRADE_FULL;
                    } else if (coopTrade.offered[c] < coopTrade.spare[c]) {
                        coopTrade.offered[c]++;
                        coopTrade.note = -1;
                        SOUND.playSound(SOUND_CURSOR);
                    }
                } else if (coopTrade.count > 0 && (PAD_PRESSED(PAD_LEFT) || PAD_REPEATED(PAD_LEFT))) {
                    c = coopTrade.list[coopTrade.cursor];
                    if (coopTrade.offered[c] > 0) {
                        coopTrade.offered[c]--;
                        coopTrade.note = -1;
                        SOUND.playSound(SOUND_CURSOR);
                    }
                } else if (PAD_PRESSED(PAD_CROSS)) {
                    if (!coopTradeRoom(&in)) {
                        coopTrade.note = COOP_TEXT_TRADE_NO_ROOM;
                    } else {
                        coopTrade.phase = COOP_TRADE_READY;
                        coopTrade.readyTheirs = theirs;
                        coopTrade.note = -1;
                        SOUND.playSound(SOUND_SELECT);
                    }
                } else if (PAD_PRESSED(PAD_CIRCLE)) {
                    SOUND.playSound(SOUND_MENU_CANCEL);
                    coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
                    break;
                }
            } else if (coopTrade.phase == COOP_TRADE_READY) {
                coopTradeOffer(&out);
                if (theirs != coopTrade.readyTheirs || !coopTradeRoom(&in)) {
                    coopTrade.phase = COOP_TRADE_OPEN; /* their offer changed: look again */
                } else if (PAD_PRESSED(PAD_CIRCLE)) {
                    coopTrade.phase = COOP_TRADE_OPEN;
                    SOUND.playSound(SOUND_MENU_CANCEL);
                } else if (PAD_PRESSED(PAD_CROSS) && in.phase >= COOP_TRADE_READY && in.phase <= COOP_TRADE_DONE &&
                           in.seen == out.offer && in.offer == theirs && (out.count > 0 || in.count > 0)) {
                    coopTrade.phase = COOP_TRADE_CONFIRM;
                    SOUND.playSound(SOUND_SELECT);
                }
            } else if (coopTrade.phase == COOP_TRADE_CONFIRM) {
                coopTradeOffer(&out);
                if (theirs != coopTrade.readyTheirs || in.phase == COOP_TRADE_OPEN) {
                    coopTrade.phase = COOP_TRADE_OPEN; /* they went back before confirming: so does this side */
                } else if ((in.phase == COOP_TRADE_CONFIRM || in.phase == COOP_TRADE_DONE) && in.seen == out.offer &&
                           in.offer == theirs) {
                    /* both confirmed the same: theirs in, mine out, once (theirs checked again: it came by network) */
                    if (!coopTradeRoom(&in)) {
                        coopTradeEnd(COOP_TRADE_NO, COOP_TEXT_TRADE_CANCELLED);
                        return;
                    }
                    for (i = 0; i < out.count; i++) {
                        GAME.cards[out.card[i]] -= out.copies[i];
                        if (GAME.cards[out.card[i]] < 0) {
                            GAME.cards[out.card[i]] = 0;
                        }
                    }
                    for (i = 0; i < in.count; i++) {
                        GAME.funcs.addCards(in.card[i], in.copies[i]);
                    }
                    native_coop_note("card trade with player %d: gave %d (%d x%d, %d x%d...), got %d (%d x%d, %d x%d...); "
                                     "card 1 now %d, card 2 now %d", coopTrade.peer + 2, out.count, out.card[0],
                                     out.copies[0], out.card[1], out.copies[1], in.count, in.card[0], in.copies[0],
                                     in.card[1], in.copies[1], GAME.cards[1], GAME.cards[2]);
                    SOUND.playSound(SOUND_MENU_CONFIRM);
                    coopTradeEnd(COOP_TRADE_DONE, COOP_TEXT_TRADE_DONE);
                }
            }
            break;
        }
    }

    /* what goes to each player: the trade's side to the one traded with, the last word a while, nothing to the rest */
    for (n = 0; n < native_coop_peer_slots() && n < COOP_GHOSTS; n++) {
        NativeCoopTrade send;

        send.session = 0;
        send.phase = COOP_TRADE_NONE;
        send.count = 0;
        send.offer = send.seen = 0;
        send.pad[0] = send.pad[1] = 0;
        if (coopRaidGo) {
            send = coopRaidGoPacket; /* the raid just started: "go", on through its battle */
        } else if (coopTrade.kind == 2 && coopTrade.phase >= COOP_TRADE_ASK && coopTrade.phase <= COOP_TRADE_CONFIRM) {
            coopRoomPacket(&send); /* the raid's room: to everyone */
        } else if (coopTrade.phase == COOP_TRADE_ASK) {
            send.session = coopTrade.session; /* waiting: to everyone */
            send.phase = COOP_TRADE_ASK;
            send.pad[0] = coopTrade.kind;
            send.pad[1] = coopTrade.kind == 2 ? coopTrade.raid : 0;
            send.offer = coopTradeHash(&send);
        } else if (coopTrade.phase >= COOP_TRADE_OPEN && coopTrade.phase <= COOP_TRADE_DONE && n == coopTrade.peer) {
            coopTradeOffer(&send);
            send.session = coopTrade.session;
            send.phase = coopTrade.phase;
            send.pad[0] = coopTrade.kind;
            send.seen = coopTradeHash(&in);
        } else if (coopTrade.linger > 0 && n == coopTrade.msgPeer) {
            send = coopTrade.last;
        }
        native_coop_trade_out(n, &send);
    }
    if (coopTrade.linger > 0) {
        coopTrade.linger--;
    }
    if (coopTrade.phase != COOP_TRADE_NONE) {
        coopTradeOffer(&out);
    }
    native_coop_tags_block(coopTrade.phase != COOP_TRADE_NONE || coopTrade.msgFrames > 0 || coopGuardMenu != NULL);
    coopTradeDraw(&out, &in); /* the custom trade window, and what the game's windows show (coopWant) */
    coopGuardMenuFrame();
}

/* Every frame of the running field: the player's state out, the others' in */
static void coopUpdate(FieldChildren *children) {
    Actor *player = children->actors[0];
    NativeCoopState now;
    NativeCoopState peer;
    Actor *ghost;
    s32 i;

    if (!g_coop || player == NULL) {
        return;
    }
    now.mode = GAME.funcs.getMode();
    now.stageFile = FIELDSTG_state.stageFile;
    now.x = player->pos.x;
    now.y = player->pos.y;
    now.z = player->z;
    now.dir = player->dir;
    now.animSet = player->animSet;
    now.depth = player->depth;
    now.hasLook = coopPlayerLook(&now.model, &now.tone);
    for (i = 0; i < (s32)sizeof now.name; i++) {
        now.name[i] = GAME.name[i];
    }
    now.name[sizeof now.name - 1] = 0;
    native_coop_publish(&now);
    if (!coopTried && ++coopWait >= COOP_WAIT) {
        coopTried = 1;
        coopCreateGhosts(children, player);
    }
    for (i = 0; i < COOP_GHOSTS; i++) {
        ghost = coopGhost[i];
        if (ghost == NULL) {
            continue;
        }
        /* an event's startWalk or anything else doesn't get to move it */
        ghost->control = NULL;
        if (ghost->substate != ACTOR_POSED) {
            ghost->setSubstate(ghost, ACTOR_POSED);
        }
        if (native_coop_peer(i, &peer) && peer.mode == now.mode && peer.stageFile == now.stageFile) {
            ghost->pos.x = peer.x;
            ghost->pos.y = peer.y;
            ghost->z = peer.z;
            ghost->dir = peer.dir;
            ghost->depth = peer.depth;
            /* another look: its file and frames again */
            if (coopGhostModel[i] != peer.model || coopGhostTone[i] != (peer.hasLook ? peer.tone : -1)) {
                coopGhostModel[i] = peer.model;
                coopGhostTone[i] = peer.hasLook ? peer.tone : -1;
                ghost->loadedSet = -1;
                ghost->reloadImage = 1;
            }
            if (ghost->animSet != peer.animSet) {
                FIELDSTG_setActorAnim(ghost, peer.animSet);
            }
            if (!coopSeen[i]) {
                coopSeen[i] = 1;
                native_coop_note("you see player %d on map %X", i + 2, now.mode);
            }
            if (g_coop_debug && ++coopWait % 100 == 0) {
                native_log("coop: player %d state %d/%X at (%d, %d) z %d anim %X/%X frame %dx%d depth %d, you at (%d, %d)",
                           i + 2, ghost->state, ghost->substate, ghost->tile.x, ghost->tile.y, ghost->z, ghost->animFile,
                           ghost->animSet, ghost->frameWidth, ghost->frameHeight, ghost->depth, player->tile.x,
                           player->tile.y);
            }
        } else {
            /* elsewhere: at tile (0, 0), which FIELDSTG_updateActor doesn't draw */
            ghost->pos.x = 0;
            ghost->pos.y = 0;
        }
    }
    coopTradeUpdate();
}
