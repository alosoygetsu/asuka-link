/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): the Guardromon's window: its drawing (the menu, the trade, the raid list, the waits). Not built alone:
   it uses what field.c and the earlier parts define. */

/* ---- drawing: the game's own glyphs (as its text windows draw them) and panels, on the field's text layer: the
   glyphs at its depth 0, the panels behind at 1 ---- */

static void coopTradeRect(s32 x, s32 y, s32 w, s32 h, s32 r, s32 g, s32 b, s32 half) {
    Layer *layer = GFX.funcs.getLayer(FIELD_LAYER_TEXT);
    u_long *ot = (u_long *)layer->getOtEntry(layer, 1);
    POLY_F4 *poly = GFX.funcs.getPrim();
    DR_TPAGE *mode;

    setPolyF4(poly);
    setRGB0(poly, r, g, b);
    setXY4(poly, x, y, x + w, y, x, y + h, x + w, y + h);
    if (half) {
        setSemiTrans(poly, 1);
    }
    addPrim(ot, poly);
    mode = (DR_TPAGE *)(poly + 1);
    if (half) { /* drawn first: the ground under it darkened (subtracted) */
        SetDrawTPage(mode, 0, 1, GetTPage(0, 2, 0, 0));
        addPrim(ot, mode);
        mode++;
    }
    GFX.funcs.setPrim(mode);
}

/* A panel: a gold frame, then (behind it, as added after) the darkened ground */
static void coopTradePanel(s32 x, s32 y, s32 w, s32 h) {
    coopTradeRect(x, y, w, 1, 0xF0, 0xB0, 0x20, 0);
    coopTradeRect(x, y + h - 1, w, 1, 0xF0, 0xB0, 0x20, 0);
    coopTradeRect(x, y, 1, h, 0xF0, 0xB0, 0x20, 0);
    coopTradeRect(x + w - 1, y, 1, h, 0xF0, 0xB0, 0x20, 0);
    coopTradeRect(x + 1, y + 1, w - 2, h - 2, 0x68, 0x68, 0x58, 1);
}

/* text (the game's, as its text files have it) at x, y in palette (PALETTE_*), no wider than maxW (0 any): its width */
static s32 coopTradeText(const u8 *text, s32 x, s32 y, s32 palette, s32 maxW) {
    Layer *layer = GFX.funcs.getLayer(FIELD_LAYER_TEXT);
    u_long *ot = (u_long *)layer->getOtEntry(layer, 0);
    TextStyle *style = &FONT.styles[1];
    const u8 *p = text;
    PrimPtr prim;
    Glyph *g;
    s32 c;
    s32 kind;
    s32 cx = 0;
    s32 tpage;

    prim.any = GFX.funcs.getPrim();
    for (;;) {
        c = FONT.decode(p, 0, style);
        kind = (c >> 8) & 0xFF;
        if (kind == 4 || *p == 0) {
            break;
        }
        if (kind == 0) {
            g = &style->glyphs[(c & 0xFF) - 4];
            p += 1;
        } else if (kind == 1) {
            g = (c & 0xFF) > 0 && (c & 0xFF) <= style->iconCount ? &style->icons[(c & 0xFF) - 1] : style->glyphs;
            p += 2;
        } else if (kind == 2) {
            p += p[1] < 11 && FONT.codeLengths[p[1]] > 0 ? FONT.codeLengths[p[1]] : 2; /* a new line: a space */
            cx += 4;
            continue;
        } else {
            g = style->glyphs;
            p += 1;
        }
        if (g->page == 0xFF) {
            g = style->glyphs;
        }
        if (maxW > 0 && cx + g->advance > maxW) {
            break;
        }
        tpage = getTPage(0, style->blend == 0xFF ? 1 : style->blend & 3, 0x140 + (g->page << 6), 0);
        setlen(prim.sprt, 4);
        setcode(prim.sprt, 0x64);
        if (style->blend != 0xFF) {
            setSemiTrans(prim.sprt, 1);
        }
        prim.sprt->r0 = prim.sprt->g0 = prim.sprt->b0 = 0x80;
        prim.sprt->x0 = x + cx + g->dx;
        prim.sprt->y0 = y + g->dy;
        prim.sprt->u0 = g->u;
        prim.sprt->v0 = g->v;
        prim.sprt->w = g->w;
        prim.sprt->h = g->h;
        prim.sprt->clut = getClut(0x140 + g->clutX, palette + g->clutY);
        addPrim(ot, prim.any);
        prim.sprt++;
        SetDrawTPage(prim.tpage, 0, 1, tpage);
        addPrim(ot, prim.any);
        prim.tpage++;
        cx += g->advance;
    }
    GFX.funcs.setPrim(prim.any);
    return cx;
}

static const u8 *coopTradeString(s32 n) {
    return (const u8 *)getString((s32 *)native_coop_text(LANGUAGE), n); /* n: a COOP_TEXT_* (include/coop_text_ids.h) */
}

/* Appends text (the game's, 0-ended) to buf of size */
static void coopTradeCat(u8 *buf, s32 size, const u8 *text) {
    s32 n = 0;

    while (n < size - 1 && buf[n] != 0) {
        n++;
    }
    while (n < size - 1 && *text != 0) {
        buf[n++] = *text++;
    }
    buf[n] = 0;
}

/* A number in the game's digits (0 is 0x04), after prefix (a game character, 0 none) */
static void coopTradeNumber(u8 *buf, s32 prefix, s32 value) {
    s32 n = 0;

    if (prefix != 0) {
        buf[n++] = prefix;
    }
    if (value >= 10) {
        buf[n++] = 0x04 + value / 10 % 10;
    }
    buf[n++] = 0x04 + value % 10;
    buf[n] = 0;
}

/* A button's icon and its word: "(X) Ready" */
static s32 coopTradeButton(s32 x, s32 y, s32 icon, s32 word) {
    u8 buf[48];

    buf[0] = 0x01;
    buf[1] = icon;
    buf[2] = 0x01; /* a space */
    buf[3] = 0x01;
    buf[4] = 0;
    coopTradeCat(buf, sizeof buf, coopTradeString(word));
    return coopTradeText(buf, x, y, PALETTE_WHITE, 0) + 12;
}
#define COOP_ICON_CROSS 0x1B
#define COOP_ICON_CIRCLE 0x2F

static const u8 *coopTradeName(s32 n, u8 *buf) {
    NativeCoopState st;
    s32 i;

    buf[0] = 0;
    if (native_coop_peer(n, &st)) {
        for (i = 0; i < 15 && st.name[i] != 0; i++) {
            buf[i] = st.name[i];
        }
        buf[i] = 0;
    }
    return buf;
}

static void coopTradeCardName(s32 card, s32 x, s32 y, s32 palette, s32 maxW) {
    coopTradeText((const u8 *)getString(FILE_CACHE.load(TEXT_FILE(TEXT_CARD_NAMES)), card), x, y, palette, maxW);
}

/* A short window at the foot: line (built), and the buttons under it */
static void coopTradeSmall(const u8 *line, s32 cross, s32 circle) {
    s32 x = 0x18;

    coopTradePanel(0x10, 0xA8, 0x120, 0x34);
    coopTradeText(line, 0x18, 0xB0, PALETTE_WHITE, 0x110);
    if (cross >= 0) {
        x += coopTradeButton(x, 0xC6, COOP_ICON_CROSS, cross);
    }
    if (circle >= 0) {
        coopTradeButton(x, 0xC6, COOP_ICON_CIRCLE, circle);
    }
}

static void coopTradeOfferRows(const NativeCoopTrade *t, s32 x, s32 y) {
    u8 num[8];
    s32 i;

    if (t->count == 0) {
        coopTradeText(coopTradeString(COOP_TEXT_TRADE_NOTHING), x, y, PALETTE_GREY, 0);
    }
    for (i = 0; i < t->count; i++) {
        coopTradeCardName(t->card[i], x, y + i * 12, PALETTE_WHITE, 0x6A);
        coopTradeNumber(num, 0x3F, t->copies[i]); /* "x3" */
        coopTradeText(num, x + 0x70, y + i * 12, PALETTE_YELLOW, 0);
    }
}

static void coopTradeStatus(s32 phase, s32 x, s32 y) {
    if (phase == COOP_TRADE_READY) {
        coopTradeText(coopTradeString(COOP_TEXT_TRADE_READY), x, y, PALETTE_BLUE, 0);
    } else if (phase == COOP_TRADE_CONFIRM || phase == COOP_TRADE_DONE) {
        coopTradeText(coopTradeString(COOP_TEXT_TRADE_CONFIRMED), x, y, PALETTE_GREEN, 0);
    }
}

static void coopTradeDraw(const NativeCoopTrade *mine, const NativeCoopTrade *in) {
    u8 buf[96];
    u8 name[16];
    u8 num[8];
    s32 i;
    s32 c;
    s32 y;
    s32 x;
    s32 both;

    if (coopTrade.phase == COOP_TRADE_NONE) {
        if (coopTrade.msgFrames > 0) {
            buf[0] = 0;
            if (coopTrade.msg == COOP_TEXT_TRADE_DECLINED) {
                coopTradeCat(buf, sizeof buf, coopTradeName(coopTrade.msgPeer, name));
            }
            coopTradeCat(buf, sizeof buf, coopTradeString(coopTrade.msg));
            coopTradeSmall(buf, -1, -1);
        }
        return;
    }
    if (coopTrade.phase == COOP_TRADE_MENU) {
        static const u8 OPTIONS[6] = {COOP_TEXT_GUARD_TALK, COOP_TEXT_GUARD_TRADE, COOP_TEXT_GUARD_BATTLE, COOP_TEXT_GUARD_DUEL, COOP_TEXT_GUARD_RAID, COOP_TEXT_GUARD_EXIT};

        coopTradePanel(0x10, 0x70, 0x120, 0x6C);
        for (i = 0; i < 6; i++) {
            if (coopTrade.menu == i) {
                coopTradeRect(0x16, 0x77 + i * 14, 0x110, 12, 0x20, 0x38, 0x80, 0);
            }
            coopTradeText(coopTradeString(OPTIONS[i]), 0x1C, 0x78 + i * 14, coopTrade.menu == i ? PALETTE_YELLOW : PALETTE_WHITE, 0);
        }
        return;
    }
    if (coopTrade.phase == COOP_TRADE_RAID_PICK) {
        s32 open[COOP_RAID_BOSSES];
        s32 count = coopRaidOpen(open);
        u8 num[8];

        coopTradePanel(0x10, 0x2C, 0x120, 0x2A + (count < COOP_RAID_ROWS ? count : COOP_RAID_ROWS) * 14);
        coopTradeText(coopTradeString(COOP_TEXT_GUARD_RAID), 0x18, 0x32, PALETTE_YELLOW, 0);
        coopTradeRect(0x18, 0x42, 0x110, 1, 0x60, 0x50, 0x20, 0);
        for (i = 0; i < COOP_RAID_ROWS && coopTrade.raidTop + i < count; i++) {
            const CoopRaidBoss *boss = &coopRaidBosses[open[coopTrade.raidTop + i]];
            s32 y = 0x48 + i * 14;
            s32 on = coopTrade.raidTop + i == coopTrade.raidCursor;

            if (on) {
                coopTradeRect(0x16, y - 1, 0x110, 12, 0x20, 0x38, 0x80, 0);
            }
            coopTradeText((const u8 *)getString(FILE_CACHE.load(TEXT_FILE(TEXT_DIGIMON_NAMES)), boss->nameId), 0x1C, y,
                          on ? PALETTE_YELLOW : PALETTE_WHITE, 0xB8);
            coopTradeText(coopTradeString(COOP_TEXT_RAID_LEVEL), 0xE0, y, PALETTE_GREY, 0);
            coopTradeNumber(num, 0, boss->level);
            coopTradeText(num, 0xF8, y, on ? PALETTE_YELLOW : PALETTE_WHITE, 0);
        }
        return;
    }
    if (coopTrade.kind >= 1 && coopTrade.phase >= COOP_TRADE_OPEN) {
        coopTradeSmall(coopTradeString(COOP_TEXT_GUARD_PREPARING), -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK && coopTrade.kind == 2) {
        /* the raid's room: "Waiting for another player... 2/4  9" */
        u8 line[96];
        u8 num[8];
        s32 secs = coopTrade.roomCount >= 2 ? (COOP_ROOM_WAIT - coopTrade.roomFrames + 59) / 60 : 0;

        line[0] = 0;
        coopTradeCat(line, sizeof line, coopTradeString(COOP_TEXT_GUARD_WAIT_OTHER));
        coopTradeCat(line, sizeof line, (const u8 *)"\x01\x01");
        coopTradeNumber(num, 0, coopTrade.roomCount);
        coopTradeCat(line, sizeof line, num);
        coopTradeCat(line, sizeof line, coopTradeString(COOP_TEXT_RAID_OF_FOUR));
        if (secs > 0) {
            coopTradeCat(line, sizeof line, (const u8 *)"\x01\x01");
            coopTradeNumber(num, 0, secs > 99 ? 99 : secs);
            coopTradeCat(line, sizeof line, num);
        }
        coopTradeSmall(line, -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK) {
        coopTradeSmall(coopTradeString(COOP_TEXT_GUARD_WAIT_OTHER), -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    /* the trade: the title and who with, the two offers, the spare cards, a note, the buttons */
    coopTradePanel(0x0C, 0x0C, 0x128, 0xDC);
    coopTradeText(coopTradeString(COOP_TEXT_TRADE_TITLE), 0x14, 0x12, PALETTE_YELLOW, 0);
    coopTradeText(coopTradeName(coopTrade.peer, name), 0xB4, 0x12, PALETTE_BLUE, 0x6C);
    coopTradeRect(0x14, 0x22, 0x118, 1, 0x60, 0x50, 0x20, 0);
    coopTradeText(coopTradeString(COOP_TEXT_TRADE_YOURS), 0x14, 0x26, PALETTE_GREEN, 0x50);
    coopTradeStatus(coopTrade.phase, 0x68, 0x26);
    coopTradeText(coopTradeString(COOP_TEXT_TRADE_THEIRS), 0xA4, 0x26, PALETTE_GREEN, 0x50);
    coopTradeStatus(in->phase, 0xF8, 0x26);
    coopTradeOfferRows(mine, 0x14, 0x34);
    coopTradeOfferRows(in, 0xA4, 0x34);
    coopTradeRect(0x14, 0x72, 0x118, 1, 0x60, 0x50, 0x20, 0);
    coopTradeText(coopTradeString(COOP_TEXT_TRADE_SPARE), 0x14, 0x76, PALETTE_GREEN, 0);
    if (coopTrade.count == 0) {
        coopTradeText(coopTradeString(COOP_TEXT_TRADE_NO_SPARE), 0x14, 0x86, PALETTE_GREY, 0);
    }
    if (coopTrade.cursor < coopTrade.top) {
        coopTrade.top = coopTrade.cursor;
    }
    if (coopTrade.cursor >= coopTrade.top + COOP_TRADE_ROWS) {
        coopTrade.top = coopTrade.cursor - COOP_TRADE_ROWS + 1;
    }
    for (i = 0; i < COOP_TRADE_ROWS && coopTrade.top + i < coopTrade.count; i++) {
        c = coopTrade.list[coopTrade.top + i];
        y = 0x86 + i * 12;
        if (coopTrade.top + i == coopTrade.cursor && coopTrade.phase == COOP_TRADE_OPEN) {
            coopTradeRect(0x12, y - 1, 0x11C, 12, 0x20, 0x38, 0x80, 0);
        }
        coopTradeCardName(c, 0x18, y, coopTrade.offered[c] != 0 ? PALETTE_YELLOW : PALETTE_WHITE, 0xB0);
        if (coopTrade.offered[c] != 0) {
            num[0] = 0x01; /* "<2>": the copies that go */
            num[1] = 0x1D;
            coopTradeNumber(num + 2, 0, coopTrade.offered[c]);
            coopTradeCat(num, sizeof num, (const u8 *)"\x01\x1E");
            coopTradeText(num, 0xD0, y, PALETTE_YELLOW, 0);
        }
        coopTradeNumber(num, 0x3F, coopTrade.spare[c]); /* "x3": the spare copies */
        coopTradeText(num, 0x108, y, PALETTE_WHITE, 0);
    }
    if (coopTrade.note >= 0) {
        coopTradeText(coopTradeString(coopTrade.note), 0x14, 0xC4, PALETTE_RED, 0);
    }
    x = 0x14;
    y = 0xD4;
    both = in->phase >= COOP_TRADE_READY && in->phase <= COOP_TRADE_DONE;
    if (coopTrade.phase == COOP_TRADE_OPEN) {
        coopTradeText(coopTradeString(COOP_TEXT_TRADE_AMOUNT), 0x14, 0xC4 + (coopTrade.note >= 0 ? -12 : 0), PALETTE_GREY, 0);
        x += coopTradeButton(x, y, COOP_ICON_CROSS, COOP_TEXT_TRADE_READY);
        coopTradeButton(x, y, COOP_ICON_CIRCLE, COOP_TEXT_TRADE_LEAVE);
    } else if (coopTrade.phase == COOP_TRADE_READY) {
        if (both) {
            x += coopTradeButton(x, y, COOP_ICON_CROSS, COOP_TEXT_TRADE_CONFIRM);
        }
        coopTradeButton(x, y, COOP_ICON_CIRCLE, COOP_TEXT_TRADE_CHANGE);
    } else {
        coopTradeText(coopTradeString(COOP_TEXT_TRADE_WAIT_DOTS), x, y, PALETTE_GREY, 0);
    }
}
