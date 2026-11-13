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

/* ---- the Guardromon's windows: the game's own, as its NPCs open them (stages/common/list_menu.inc.c, WSTAG210's
   guide): a list (MENU_SPRITES' frame for 5 to 8 rows, text windows, the cursor) and the message box at the foot
   (sprite 0x45, a text window), each opening and closing as theirs, with their sounds. Each frame coopTradeDraw says
   what should show (coopWant); the task opens, changes or shuts each part to match ---- */

#define COOP_GUARD_TALK 0
#define COOP_GUARD_TRADE 1
#define COOP_GUARD_BATTLE 2
#define COOP_GUARD_DUEL 3
#define COOP_GUARD_RAID 4
#define COOP_GUARD_EXIT 5
#define COOP_GUARD_ROWS 6

#define COOP_LIST_NONE 0
#define COOP_LIST_MENU 1 /* the Guardromon's choice */
#define COOP_LIST_RAIDS 2 /* the raid's boss */
#define COOP_LIST_ROWS 8 /* the game's list: up to 8 */
#define COOP_BOX_SIZE 160

/* What the menu offers, row by row (COOP_GUARD_*): how many. The card duel only once the game has opened the cards
   (the Folder Bag, item 0x192: the field menu's Cards with it, main/menu/field_menu.c) */
static s32 coopGuardOptions(s32 *rows) {
    s32 n = 0;

    rows[n++] = COOP_GUARD_TALK;
    rows[n++] = COOP_GUARD_TRADE;
    rows[n++] = COOP_GUARD_BATTLE;
    if (GAME.items[0x192] != 0) {
        rows[n++] = COOP_GUARD_DUEL;
    }
    rows[n++] = COOP_GUARD_RAID;
    rows[n++] = COOP_GUARD_EXIT;
    return n;
}

/* What should show this frame (coopTradeDraw fills it) */
static struct {
    s32 list; /* COOP_LIST_*: another one shuts the one shown and opens itself */
    s32 rows;
    const u8 *row[COOP_LIST_ROWS];
    s32 cursor; /* its row */
    u8 box[COOP_BOX_SIZE]; /* the message box's text, empty: none */
} coopWant;

#define COOP_PART_SHUT 0
#define COOP_PART_OPENING 1
#define COOP_PART_OPEN 2
#define COOP_PART_CLOSING 3

typedef struct CoopGuardMenu {
    TASK_HEADER(CoopGuardMenu);
    s32 list, rows; /* the list shown */
    s32 listPart, boxPart; /* COOP_PART_* */
    const u8 *shown[COOP_LIST_ROWS];
    u8 box[COOP_BOX_SIZE];
    PanelAnim tweens[2]; /* the list's width, the box's */
} CoopGuardMenu;

typedef struct CoopGuardMenuChildren {
    TextWindow *options[COOP_LIST_ROWS];
    Cursor *cursor;
    TextWindow *message;
} CoopGuardMenuChildren;

/* MENU_SPRITES' frame of the list's window, by the number of rows (5 to 8: WSTAG210's listWindowFrames) */
static const s32 coopGuardFrames[4] = {28, 27, 25, 36};

static s32 coopBoxSame(const u8 *a, const u8 *b) {
    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

/* The list: opens what coopWant asks, changes its rows and cursor, shuts it when it asks none or another */
static void coopGuardList(CoopGuardMenu *task, CoopGuardMenuChildren *children) {
    s32 i;

    switch (task->listPart) {
    case COOP_PART_SHUT:
        if (coopWant.list != COOP_LIST_NONE) {
            task->list = coopWant.list;
            task->rows = coopWant.rows;
            FIELDSTG_tweenStart(&task->tweens[0], 1);
            task->listPart = COOP_PART_OPENING;
        }
        break;
    case COOP_PART_OPENING:
        if (FIELDSTG_tweenUpdate(&task->tweens[0])) {
            for (i = 0; i < COOP_LIST_ROWS; i++) {
                task->shown[i] = NULL;
            }
            task->listPart = COOP_PART_OPEN;
        }
        break;
    case COOP_PART_OPEN:
        if (coopWant.list != task->list || coopWant.rows != task->rows) {
            for (i = 0; i < task->rows; i++) {
                children->options[i]->setVisible(children->options[i], 0);
            }
            children->cursor->setVisible(children->cursor, 0);
            FIELDSTG_tweenStart(&task->tweens[0], 0);
            task->listPart = COOP_PART_CLOSING;
            break;
        }
        for (i = 0; i < task->rows; i++) {
            if (task->shown[i] != coopWant.row[i]) {
                task->shown[i] = coopWant.row[i];
                children->options[i]->setString(children->options[i], (char *)coopWant.row[i], -1);
            }
        }
        children->cursor->setPos(children->cursor, 0xAF, 0x21 + coopWant.cursor * 14);
        children->cursor->setVisible(children->cursor, 1);
        break;
    case COOP_PART_CLOSING:
        if (FIELDSTG_tweenUpdate(&task->tweens[0])) {
            task->listPart = COOP_PART_SHUT;
        }
        break;
    }
}

/* The message box: the same, with coopWant.box */
static void coopGuardBox(CoopGuardMenu *task, CoopGuardMenuChildren *children) {
    s32 i;

    switch (task->boxPart) {
    case COOP_PART_SHUT:
        if (coopWant.box[0] != 0) {
            FIELDSTG_tweenStart(&task->tweens[1], 1);
            task->boxPart = COOP_PART_OPENING;
        }
        break;
    case COOP_PART_OPENING:
        if (FIELDSTG_tweenUpdate(&task->tweens[1])) {
            task->box[0] = 0xFF; /* none written yet */
            task->box[1] = 0;
            task->boxPart = COOP_PART_OPEN;
        }
        break;
    case COOP_PART_OPEN:
        if (coopWant.box[0] == 0) {
            children->message->setVisible(children->message, 0);
            FIELDSTG_tweenStart(&task->tweens[1], 0);
            task->boxPart = COOP_PART_CLOSING;
        } else if (!coopBoxSame(task->box, coopWant.box)) {
            for (i = 0; i < COOP_BOX_SIZE; i++) {
                task->box[i] = coopWant.box[i];
            }
            children->message->setString(children->message, (char *)task->box, -1);
        }
        break;
    case COOP_PART_CLOSING:
        if (FIELDSTG_tweenUpdate(&task->tweens[1])) {
            task->boxPart = COOP_PART_SHUT;
        }
        break;
    }
}

static void coopGuardMenuRun(CoopGuardMenu *task, CoopGuardMenuChildren *children) {
    SpriteDrawer drawer;
    s32 i;

    switch (task->state) {
    case TASK_INIT:
    default:
        task->nextState(task);
        task->tweens[0].duration = 10;
        task->tweens[1].duration = 10;
        for (i = 0; i < COOP_LIST_ROWS; i++) {
            children->options[i] = createTextWindow(FIELD_LAYER_MAP, 1, 0xBD, 0x21 + i * 14);
            children->options[i]->setDepth(children->options[i], 1);
        }
        children->cursor = createCursor(FIELD_LAYER_MAP, 1, 0xAF, 0x21);
        children->cursor->setVisible(children->cursor, 0);
        children->message = createTextWindow(FIELD_LAYER_MAP, 1, 0x12, 0xB0);
        children->message->setLines(children->message, 3);
        break;
    case TASK_RUN:
        coopGuardList(task, children);
        coopGuardBox(task, children);
        if (task->listPart == COOP_PART_SHUT && task->boxPart == COOP_PART_SHUT && coopWant.list == COOP_LIST_NONE &&
            coopWant.box[0] == 0) {
            task->state = TASK_KILL;
            break;
        }
        initSpriteDrawer(&drawer);
        drawer.setLayerId(FIELD_LAYER_MAP, 2);
        drawer.setTexture(0x140, 0);
        drawer.setFollowScroll(0);
        if (task->listPart != COOP_PART_SHUT && task->tweens[0].level != 0) {
            if (task->tweens[0].level != ONE) {
                drawer.setScale(task->tweens[0].level, ONE, ONE);
                drawer.setPivot(0x140, 0x56);
            }
            drawer.draw(FILE_CACHE.getEntry(FILE_MENU_SPRITES << 16), coopGuardFrames[(task->rows < 5 ? 5 : task->rows) - 5],
                        0xA8, 0x18);
        }
        if (task->boxPart != COOP_PART_SHUT && task->tweens[1].level != 0) {
            drawer.setScale(task->tweens[1].level, ONE, ONE);
            drawer.setPivot(0, 0xC3); /* as FIELDSTG's questions open theirs (fieldstg/choice.c) */
            drawer.draw(FILE_CACHE.getEntry(FILE_MENU_SPRITES << 16), 0x45, 0, 0xAC);
        }
        break;
    case TASK_DONE:
    case TASK_KILL:
        break;
    }
}

/* list (COOP_LIST_*) open and taking buttons: the game's menus take none while opening */
static s32 coopGuardListReady(s32 list) {
    return coopGuardMenu != NULL && coopGuardMenu->state == TASK_RUN && coopGuardMenu->listPart == COOP_PART_OPEN &&
           coopGuardMenu->list == list && coopWant.list == list;
}

/* Each frame, after coopTradeDraw: the windows as coopWant says */
static void coopGuardMenuFrame(void) {
    if (coopGuardMenu == NULL && (coopWant.list != COOP_LIST_NONE || coopWant.box[0] != 0)) {
        coopGuardMenu = createTask(coopGuardMenuRun, sizeof(CoopGuardMenu), sizeof(CoopGuardMenuChildren));
    }
    if (coopGuardMenu != NULL) {
        coopGuardMenu = (CoopGuardMenu *)TASK_REGISTRY.funcs.run((Task *)coopGuardMenu);
    }
}

/* The message box's text: line, then on a second line a button's icon and its word (-1 none) */
static void coopWantBox(const u8 *line, s32 icon, s32 word) {
    u8 button[7] = {0x02, 0x01, 0x01, 0, 0x01, 0x01, 0}; /* a new line, the icon, a space */

    coopWant.box[0] = 0;
    coopTradeCat(coopWant.box, COOP_BOX_SIZE, line);
    if (word >= 0) {
        button[3] = icon;
        coopTradeCat(coopWant.box, COOP_BOX_SIZE, button);
        coopTradeCat(coopWant.box, COOP_BOX_SIZE, coopTradeString(word));
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

    /* the game's windows (coopGuardMenuRun): what they show this frame */
    coopWant.list = COOP_LIST_NONE;
    coopWant.rows = 0;
    coopWant.box[0] = 0;
    if (coopTrade.phase == COOP_TRADE_NONE) {
        if (coopTrade.msgFrames > 0) {
            buf[0] = 0;
            if (coopTrade.msg == COOP_TEXT_TRADE_DECLINED) {
                coopTradeCat(buf, sizeof buf, coopTradeName(coopTrade.msgPeer, name));
            }
            coopTradeCat(buf, sizeof buf, coopTradeString(coopTrade.msg));
            coopWantBox(buf, 0, -1);
        }
        return;
    }
    if (coopTrade.phase == COOP_TRADE_MENU) {
        static const u8 TEXTS[COOP_GUARD_ROWS] = {COOP_TEXT_GUARD_TALK, COOP_TEXT_GUARD_TRADE, COOP_TEXT_GUARD_BATTLE,
                                                  COOP_TEXT_GUARD_DUEL, COOP_TEXT_GUARD_RAID, COOP_TEXT_GUARD_EXIT};
        s32 rows[COOP_GUARD_ROWS];

        coopWant.list = COOP_LIST_MENU;
        coopWant.rows = coopGuardOptions(rows);
        for (i = 0; i < coopWant.rows; i++) {
            coopWant.row[i] = coopTradeString(TEXTS[rows[i]]);
        }
        coopWant.cursor = coopTrade.menu;
        return;
    }
    if (coopTrade.phase == COOP_TRADE_RAID_PICK) {
        /* the bosses in the list, the one under the cursor (its whole name) and its level in the box */
        s32 open[COOP_RAID_BOSSES];
        s32 count = coopRaidOpen(open);
        const u8 *bossName;

        coopWant.list = COOP_LIST_RAIDS;
        for (i = 0; i < COOP_RAID_ROWS && coopTrade.raidTop + i < count; i++) {
            coopWant.row[i] = (const u8 *)getString(FILE_CACHE.load(TEXT_FILE(TEXT_DIGIMON_NAMES)),
                                                    coopRaidBosses[open[coopTrade.raidTop + i]].nameId);
        }
        coopWant.rows = i;
        coopWant.cursor = coopTrade.raidCursor - coopTrade.raidTop;
        bossName = (const u8 *)getString(FILE_CACHE.load(TEXT_FILE(TEXT_DIGIMON_NAMES)),
                                         coopRaidBosses[open[coopTrade.raidCursor]].nameId);
        buf[0] = 0;
        coopTradeCat(buf, sizeof buf, bossName);
        coopTradeCat(buf, sizeof buf, (const u8 *)"\x01\x01\x01\x01");
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_RAID_LEVEL));
        coopTradeCat(buf, sizeof buf, (const u8 *)"\x01\x01"); /* "Lv 17" */
        coopTradeNumber(num, 0, coopRaidBosses[open[coopTrade.raidCursor]].level);
        coopTradeCat(buf, sizeof buf, num);
        coopWantBox(buf, COOP_ICON_CIRCLE, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.kind >= 1 && coopTrade.phase >= COOP_TRADE_OPEN) {
        coopWantBox(coopTradeString(COOP_TEXT_GUARD_PREPARING), COOP_ICON_CIRCLE, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK && coopTrade.kind == 2) {
        /* the raid's room: "Waiting for another player... 2/4  9" */
        u8 line[96];
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
        coopWantBox(line, COOP_ICON_CIRCLE, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK) {
        coopWantBox(coopTradeString(COOP_TEXT_GUARD_WAIT_OTHER), COOP_ICON_CIRCLE, COOP_TEXT_TRADE_LEAVE);
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
