/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): the Guardromon's windows (the menu,
   the raid list, the waits, the trade), all of them the game's own. Not built alone: it uses what field.c and the
   earlier parts define. */

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

#define COOP_ICON_CROSS 0x1B
#define COOP_ICON_CIRCLE 0x2F
#define COOP_SPACE "\x01\x01" /* the game's space */

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

static const u8 *coopCardName(s32 card) {
    return (const u8 *)getString(FILE_CACHE.load(TEXT_FILE(TEXT_CARD_NAMES)), card);
}

/* ---- the Guardromon's windows: the game's own, as its NPCs open them (stages/common/list_menu.inc.c, WSTAG210's
   guide): lists (MENU_SPRITES' frame for 5 to 8 rows, text windows, the cursor) and the message box at the foot
   (sprite 0x45, a text window), each opening and closing as theirs, with their sounds. The right list is the game's,
   with the cursor; the left one, the same frame mirrored in place, has none. Their grounds are Asuka Link's lilac:
   their CLUTs copied and turned (native_coop_vram_lilac). Each frame coopTradeDraw says what should show (coopWant);
   the task opens, changes or shuts each part to match ---- */

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
#define COOP_LIST_SPARE 3 /* the trade: this player's spare cards */
#define COOP_LIST_THEIRS 4 /* the trade: the other's offer */
#define COOP_LIST_ROWS 8 /* the game's list: up to 8 */
#define COOP_ROW_SIZE 48
#define COOP_BOX_SIZE 160
#define COOP_SIDES 2 /* 0 the right list, 1 the left */

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

typedef struct CoopWantList {
    s32 list; /* COOP_LIST_*: another one shuts the one shown and opens itself */
    s32 rows;
    u8 row[COOP_LIST_ROWS][COOP_ROW_SIZE];
    u8 palette[COOP_LIST_ROWS]; /* PALETTE_* */
    s32 cursor; /* its row, -1 none (the right list only) */
} CoopWantList;

/* What should show this frame (coopTradeDraw fills it) */
static struct {
    CoopWantList sides[COOP_SIDES];
    u8 box[COOP_BOX_SIZE]; /* the message box's text, empty: none */
} coopWant;

#define COOP_PART_SHUT 0
#define COOP_PART_OPENING 1
#define COOP_PART_OPEN 2
#define COOP_PART_CLOSING 3

typedef struct CoopShownList {
    s32 list, rows; /* the list shown */
    s32 part; /* COOP_PART_* */
    u8 row[COOP_LIST_ROWS][COOP_ROW_SIZE];
    u8 palette[COOP_LIST_ROWS];
    PanelAnim tween; /* its width */
} CoopShownList;

typedef struct CoopGuardMenu {
    TASK_HEADER(CoopGuardMenu);
    CoopShownList sides[COOP_SIDES];
    s32 boxPart;
    u8 box[COOP_BOX_SIZE];
    PanelAnim boxTween;
} CoopGuardMenu;

typedef struct CoopGuardMenuChildren {
    TextWindow *options[COOP_SIDES][COOP_LIST_ROWS];
    Cursor *cursor;
    TextWindow *message;
} CoopGuardMenuChildren;

/* MENU_SPRITES' frame of the list's window, by the number of rows (5 to 8: WSTAG210's listWindowFrames) */
static const s32 coopGuardFrames[4] = {28, 27, 25, 36};
#define COOP_BOX_FRAME 0x45
#define COOP_LIST_X(side) ((side) == 0 ? 0xA8 : 0x00) /* the frame's x: the game's, and the left one */
#define COOP_ROW_X(side) (COOP_LIST_X(side) + 0x15)

static s32 coopSame(const u8 *a, const u8 *b) {
    while (*a != 0 && *a == *b) {
        a++;
        b++;
    }
    return *a == *b;
}

static void coopCopy(u8 *to, const u8 *from, s32 size) {
    s32 n = 0;

    while (n < size - 1 && from[n] != 0) {
        to[n] = from[n];
        n++;
    }
    to[n] = 0;
}

/* ---- the lilac CLUTs: the frames' 4-bit parts' CLUTs, copied once a field into a free box of VRAM and turned
   (native_coop_vram_lilac); the drawer's CLUT base moved so each part finds its copy ---- */

static s32 coopLilacX, coopLilacY; /* the drawer's CLUT base for the copies; coopLilacOn (coop_trade.inc.c) */

static void coopLilacPrepare(void) {
    static const s32 FRAMES[5] = {28, 27, 25, 36, COOP_BOX_FRAME};
    s32 *sheet = (s32 *)FILE_CACHE.getEntry(FILE_MENU_SPRITES << 16);
    u8 *ids;
    SpritePart *parts;
    s32 x0 = 0x7FFF, y0 = 0x7FFF, x1 = -1, y1 = -1;
    s32 f, i, k, x, y, px, py, w, h;

    coopLilacOn = -1; /* tried: not again this field */
    if (sheet == NULL) {
        return;
    }
    ids = (u8 *)sheet + sheet[1];
    parts = (SpritePart *)((u8 *)sheet + sheet[0]);
    for (f = 0; f < 5; f++) {
        s16 *p;
        s32 count, row;

        for (i = 0; i < 200 && ids[i] != FRAMES[f]; i++) {
        }
        if (i == 200) {
            return;
        }
        p = (s16 *)((u8 *)sheet + sheet[i + 2]);
        count = p[0];
        row = p[1];
        p += 3;
        for (k = 0; k < count; k++, p += 3) {
            SpritePart *part = &parts[p[0]];

            if (part->mode != 0) {
                continue; /* 8-bit: the other CLUT (none in these frames) */
            }
            x0 = part->clutX < x0 ? part->clutX : x0;
            y0 = part->clutY + row < y0 ? part->clutY + row : y0;
            x1 = part->clutX + 16 > x1 ? part->clutX + 16 : x1;
            y1 = part->clutY + row + 1 > y1 ? part->clutY + row + 1 : y1;
        }
    }
    w = x1 - x0;
    h = y1 - y0;
    if (w <= 0 || h <= 0 || w > 256 || h > 64 || !native_coop_find_vram_any(w, h, &x, &y, &px, &py)) {
        native_log("coop: no VRAM for the windows' lilac (%d x %d)", w, h);
        return;
    }
    native_coop_vram_reserve(x, y, w, h);
    native_coop_vram_mine(x, y, w, h);
    native_coop_vram_lilac(0x140 + x0, y0, x, y, w, h); /* the sheet's CLUTs are at its texture, (0x140, 0) */
    coopLilacX = x - x0;
    coopLilacY = y - y0;
    coopLilacOn = 1;
    if (g_coop_debug) {
        native_log("coop: the windows' lilac CLUTs: %d x %d from (%d, %d) at (%d, %d)", w, h, 0x140 + x0, y0, x, y);
    }
}

/* A frame of the menu sheet, the game's way, its ground lilac */
static void coopDrawFrame(s32 frame, s32 x, s32 y, s32 level, s32 pivotX, s32 pivotY) {
    SpriteDrawer drawer;

    initSpriteDrawer(&drawer);
    drawer.setLayerId(FIELD_LAYER_MAP, 2);
    drawer.setTexture(0x140, 0);
    if (coopLilacOn > 0) {
        drawer.clutX = coopLilacX;
        drawer.clutY = coopLilacY;
    }
    drawer.setFollowScroll(0);
    if (level != ONE) {
        drawer.setScale(level, ONE, ONE);
        drawer.setPivot(pivotX, pivotY);
    }
    drawer.draw(FILE_CACHE.getEntry(FILE_MENU_SPRITES << 16), frame, x, y);
}

/* A list: opens what coopWant asks, changes its rows and cursor, shuts it when it asks none or another */
static void coopGuardList(CoopGuardMenu *task, CoopGuardMenuChildren *children, s32 side) {
    CoopShownList *shown = &task->sides[side];
    CoopWantList *want = &coopWant.sides[side];
    TextWindow **options = children->options[side];
    s32 i;

    switch (shown->part) {
    case COOP_PART_SHUT:
        if (want->list != COOP_LIST_NONE) {
            shown->list = want->list;
            shown->rows = want->rows;
            FIELDSTG_tweenStart(&shown->tween, 1);
            shown->part = COOP_PART_OPENING;
        }
        break;
    case COOP_PART_OPENING:
        if (FIELDSTG_tweenUpdate(&shown->tween)) {
            for (i = 0; i < COOP_LIST_ROWS; i++) {
                shown->row[i][0] = 0xFF; /* none written yet */
                shown->row[i][1] = 0;
            }
            shown->part = COOP_PART_OPEN;
        }
        break;
    case COOP_PART_OPEN:
        if (want->list != shown->list || want->rows != shown->rows) {
            for (i = 0; i < shown->rows; i++) {
                options[i]->setVisible(options[i], 0);
            }
            if (side == 0) {
                children->cursor->setVisible(children->cursor, 0);
            }
            FIELDSTG_tweenStart(&shown->tween, 0);
            shown->part = COOP_PART_CLOSING;
            break;
        }
        for (i = 0; i < shown->rows; i++) {
            if (!coopSame(shown->row[i], want->row[i]) || shown->palette[i] != want->palette[i]) {
                coopCopy(shown->row[i], want->row[i], COOP_ROW_SIZE);
                shown->palette[i] = want->palette[i];
                options[i]->setPalette(options[i], want->palette[i]);
                options[i]->setString(options[i], (char *)shown->row[i], -1);
            }
        }
        if (side == 0) {
            if (want->cursor >= 0) {
                children->cursor->setPos(children->cursor, 0xAF, 0x21 + want->cursor * 14);
            }
            children->cursor->setVisible(children->cursor, want->cursor >= 0);
        }
        break;
    case COOP_PART_CLOSING:
        if (FIELDSTG_tweenUpdate(&shown->tween)) {
            shown->part = COOP_PART_SHUT;
        }
        break;
    }
}

/* The message box: the same, with coopWant.box */
static void coopGuardBox(CoopGuardMenu *task, CoopGuardMenuChildren *children) {
    switch (task->boxPart) {
    case COOP_PART_SHUT:
        if (coopWant.box[0] != 0) {
            FIELDSTG_tweenStart(&task->boxTween, 1);
            task->boxPart = COOP_PART_OPENING;
        }
        break;
    case COOP_PART_OPENING:
        if (FIELDSTG_tweenUpdate(&task->boxTween)) {
            task->box[0] = 0xFF; /* none written yet */
            task->box[1] = 0;
            task->boxPart = COOP_PART_OPEN;
        }
        break;
    case COOP_PART_OPEN:
        if (coopWant.box[0] == 0) {
            children->message->setVisible(children->message, 0);
            FIELDSTG_tweenStart(&task->boxTween, 0);
            task->boxPart = COOP_PART_CLOSING;
        } else if (!coopSame(task->box, coopWant.box)) {
            coopCopy(task->box, coopWant.box, COOP_BOX_SIZE);
            children->message->setString(children->message, (char *)task->box, -1);
        }
        break;
    case COOP_PART_CLOSING:
        if (FIELDSTG_tweenUpdate(&task->boxTween)) {
            task->boxPart = COOP_PART_SHUT;
        }
        break;
    }
}

static void coopGuardMenuRun(CoopGuardMenu *task, CoopGuardMenuChildren *children) {
    CoopShownList *shown;
    s32 side;
    s32 i;

    switch (task->state) {
    case TASK_INIT:
    default:
        task->nextState(task);
        if (coopLilacOn == 0) {
            coopLilacPrepare();
        }
        task->boxTween.duration = 10;
        for (side = 0; side < COOP_SIDES; side++) {
            task->sides[side].tween.duration = 10;
            for (i = 0; i < COOP_LIST_ROWS; i++) {
                children->options[side][i] = createTextWindow(FIELD_LAYER_MAP, 1, COOP_ROW_X(side), 0x21 + i * 14);
                children->options[side][i]->setDepth(children->options[side][i], 1);
            }
        }
        children->cursor = createCursor(FIELD_LAYER_MAP, 1, 0xAF, 0x21);
        children->cursor->setVisible(children->cursor, 0);
        children->message = createTextWindow(FIELD_LAYER_MAP, 1, 0x12, 0xB0);
        children->message->setLines(children->message, 3);
        break;
    case TASK_RUN:
        for (side = 0; side < COOP_SIDES; side++) {
            coopGuardList(task, children, side);
        }
        coopGuardBox(task, children);
        if (task->sides[0].part == COOP_PART_SHUT && task->sides[1].part == COOP_PART_SHUT &&
            task->boxPart == COOP_PART_SHUT && coopWant.sides[0].list == COOP_LIST_NONE &&
            coopWant.sides[1].list == COOP_LIST_NONE && coopWant.box[0] == 0) {
            task->state = TASK_KILL;
            break;
        }
        for (side = 0; side < COOP_SIDES; side++) {
            shown = &task->sides[side];
            if (shown->part != COOP_PART_SHUT && shown->tween.level != 0) {
                coopDrawFrame(coopGuardFrames[(shown->rows < 5 ? 5 : shown->rows) - 5], COOP_LIST_X(side), 0x18,
                              shown->tween.level, side == 0 ? 0x140 : 0, 0x56);
            }
        }
        if (task->boxPart != COOP_PART_SHUT && task->boxTween.level != 0) {
            /* opening from the left, as FIELDSTG's questions open theirs (fieldstg/choice.c) */
            coopDrawFrame(COOP_BOX_FRAME, 0, 0xAC, task->boxTween.level, 0, 0xC3);
        }
        break;
    case TASK_DONE:
    case TASK_KILL:
        break;
    }
}

/* list (COOP_LIST_*) open on the right and taking buttons: the game's menus take none while opening */
static s32 coopGuardListReady(s32 list) {
    return coopGuardMenu != NULL && coopGuardMenu->state == TASK_RUN &&
           coopGuardMenu->sides[0].part == COOP_PART_OPEN && coopGuardMenu->sides[0].list == list &&
           coopWant.sides[0].list == list;
}

/* Each frame, after coopTradeDraw: the windows as coopWant says */
static void coopGuardMenuFrame(void) {
    if (coopGuardMenu == NULL &&
        (coopWant.sides[0].list != COOP_LIST_NONE || coopWant.sides[1].list != COOP_LIST_NONE || coopWant.box[0] != 0)) {
        coopGuardMenu = createTask(coopGuardMenuRun, sizeof(CoopGuardMenu), sizeof(CoopGuardMenuChildren));
    }
    if (coopGuardMenu != NULL) {
        coopGuardMenu = (CoopGuardMenu *)TASK_REGISTRY.funcs.run((Task *)coopGuardMenu);
    }
}

/* A list's row: text in palette */
static void coopWantRow(s32 side, s32 i, const u8 *text, s32 palette) {
    coopCopy(coopWant.sides[side].row[i], text, COOP_ROW_SIZE);
    coopWant.sides[side].palette[i] = palette;
}

/* A button for the box: its icon and its word ("(X) Ready"), after what's there */
static void coopWantButton(s32 icon, s32 word) {
    u8 button[5] = {0x01, 0, 0x01, 0x01, 0}; /* the icon, a space */

    button[1] = icon;
    coopTradeCat(coopWant.box, COOP_BOX_SIZE, button);
    coopTradeCat(coopWant.box, COOP_BOX_SIZE, coopTradeString(word));
}

/* The message box's text: line, then on a second line the buttons (cross's word, circle's; -1 none) */
static void coopWantBox(const u8 *line, s32 cross, s32 circle) {
    static const u8 NEW_LINE[3] = {0x02, 0x01, 0};

    coopWant.box[0] = 0;
    coopTradeCat(coopWant.box, COOP_BOX_SIZE, line);
    if (cross >= 0 || circle >= 0) {
        coopTradeCat(coopWant.box, COOP_BOX_SIZE, NEW_LINE);
    }
    if (cross >= 0) {
        coopWantButton(COOP_ICON_CROSS, cross);
    }
    if (cross >= 0 && circle >= 0) {
        coopTradeCat(coopWant.box, COOP_BOX_SIZE, (const u8 *)COOP_SPACE COOP_SPACE COOP_SPACE);
    }
    if (circle >= 0) {
        coopWantButton(COOP_ICON_CIRCLE, circle);
    }
}

/* ---- what shows, phase by phase ---- */

/* The trade: on the right the spare cards (the cursor, "<2>" by the copies offered), on the left the other's offer
   (who, their cards, ready or confirmed), in the box the card under the cursor, a note and the buttons */
static void coopTradeWant(const NativeCoopTrade *in) {
    CoopWantList *spare = &coopWant.sides[0];
    CoopWantList *theirs = &coopWant.sides[1];
    u8 buf[96];
    u8 num[8];
    s32 both = in->phase >= COOP_TRADE_READY && in->phase <= COOP_TRADE_DONE;
    s32 c;
    s32 i;

    if (coopTrade.cursor < coopTrade.top) {
        coopTrade.top = coopTrade.cursor;
    }
    if (coopTrade.cursor >= coopTrade.top + COOP_LIST_ROWS) {
        coopTrade.top = coopTrade.cursor - COOP_LIST_ROWS + 1;
    }
    spare->list = COOP_LIST_SPARE;
    spare->rows = coopTrade.count == 0 ? 1 : coopTrade.count < COOP_LIST_ROWS ? coopTrade.count : COOP_LIST_ROWS;
    spare->cursor = coopTrade.count > 0 && coopTrade.phase == COOP_TRADE_OPEN ? coopTrade.cursor - coopTrade.top : -1;
    if (coopTrade.count == 0) {
        coopWantRow(0, 0, coopTradeString(COOP_TEXT_TRADE_NO_SPARE), PALETTE_GREY);
    }
    for (i = 0; i < spare->rows && coopTrade.top + i < coopTrade.count; i++) {
        c = coopTrade.list[coopTrade.top + i];
        buf[0] = 0;
        coopTradeCat(buf, sizeof buf, coopCardName(c));
        if (coopTrade.offered[c] != 0) {
            num[0] = 0x01; /* " <2>": the copies that go */
            num[1] = 0x1D;
            coopTradeNumber(num + 2, 0, coopTrade.offered[c]);
            coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE);
            coopTradeCat(buf, sizeof buf, num);
            coopTradeCat(buf, sizeof buf, (const u8 *)"\x01\x1E");
        }
        coopWantRow(0, i, buf, coopTrade.offered[c] != 0 ? PALETTE_YELLOW : PALETTE_WHITE);
    }

    /* the left: always 8 rows, so it stays open while their offer changes */
    theirs->list = COOP_LIST_THEIRS;
    theirs->rows = COOP_LIST_ROWS;
    theirs->cursor = -1;
    coopWantRow(1, 0, coopTradeName(coopTrade.peer, buf), PALETTE_BLUE);
    coopWantRow(1, 1, coopTradeString(COOP_TEXT_TRADE_THEIRS), PALETTE_GREEN);
    for (i = 0; i < 5; i++) {
        if (i < in->count) {
            buf[0] = 0;
            coopTradeCat(buf, sizeof buf, coopCardName(in->card[i]));
            coopTradeNumber(num, 0x3F, in->copies[i]); /* " x3" */
            coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE);
            coopTradeCat(buf, sizeof buf, num);
            coopWantRow(1, 2 + i, buf, PALETTE_WHITE);
        } else {
            coopWantRow(1, 2 + i, i == 0 ? coopTradeString(COOP_TEXT_TRADE_NOTHING) : (const u8 *)"",
                        PALETTE_GREY);
        }
    }
    coopWantRow(1, 7,
                in->phase == COOP_TRADE_READY ? coopTradeString(COOP_TEXT_TRADE_READY)
                : in->phase == COOP_TRADE_CONFIRM || in->phase == COOP_TRADE_DONE
                    ? coopTradeString(COOP_TEXT_TRADE_CONFIRMED)
                    : (const u8 *)"",
                in->phase == COOP_TRADE_READY ? PALETTE_BLUE : PALETTE_GREEN);

    /* the box: a note, or the card under the cursor ("Agumon x3   < >: amount"), or this side's word; the buttons */
    buf[0] = 0;
    if (coopTrade.note >= 0) {
        coopTradeCat(buf, sizeof buf, coopTradeString(coopTrade.note));
    } else if (coopTrade.phase == COOP_TRADE_OPEN && coopTrade.count > 0) {
        c = coopTrade.list[coopTrade.cursor];
        coopTradeCat(buf, sizeof buf, coopCardName(c));
        coopTradeNumber(num, 0x3F, coopTrade.spare[c]); /* " x3": the spare copies */
        coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE);
        coopTradeCat(buf, sizeof buf, num);
        coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE COOP_SPACE COOP_SPACE);
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_TRADE_AMOUNT));
    } else if (coopTrade.phase == COOP_TRADE_OPEN) {
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_TRADE_TITLE));
    } else if (coopTrade.phase == COOP_TRADE_READY) {
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_TRADE_READY));
    } else {
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_TRADE_CONFIRMED));
    }
    if (coopTrade.phase == COOP_TRADE_OPEN) {
        coopWantBox(buf, COOP_TEXT_TRADE_READY, COOP_TEXT_TRADE_LEAVE);
    } else if (coopTrade.phase == COOP_TRADE_READY) {
        coopWantBox(buf, both ? COOP_TEXT_TRADE_CONFIRM : -1, COOP_TEXT_TRADE_CHANGE);
    } else {
        coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE COOP_SPACE COOP_SPACE);
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_TRADE_WAIT_DOTS));
        coopWantBox(buf, -1, -1);
    }
}

static void coopTradeDraw(const NativeCoopTrade *mine, const NativeCoopTrade *in) {
    CoopWantList *list = &coopWant.sides[0];
    u8 buf[96];
    u8 name[16];
    u8 num[8];
    s32 i;

    (void)mine;
    for (i = 0; i < COOP_SIDES; i++) {
        coopWant.sides[i].list = COOP_LIST_NONE;
        coopWant.sides[i].rows = 0;
        coopWant.sides[i].cursor = -1;
    }
    coopWant.box[0] = 0;
    if (coopTrade.phase == COOP_TRADE_NONE) {
        if (coopTrade.msgFrames > 0) {
            buf[0] = 0;
            if (coopTrade.msg == COOP_TEXT_TRADE_DECLINED) {
                coopTradeCat(buf, sizeof buf, coopTradeName(coopTrade.msgPeer, name));
            }
            coopTradeCat(buf, sizeof buf, coopTradeString(coopTrade.msg));
            coopWantBox(buf, -1, -1);
        }
        return;
    }
    if (coopTrade.phase == COOP_TRADE_MENU) {
        static const u8 TEXTS[COOP_GUARD_ROWS] = {COOP_TEXT_GUARD_TALK, COOP_TEXT_GUARD_TRADE, COOP_TEXT_GUARD_BATTLE,
                                                  COOP_TEXT_GUARD_DUEL, COOP_TEXT_GUARD_RAID, COOP_TEXT_GUARD_EXIT};
        s32 rows[COOP_GUARD_ROWS];

        list->list = COOP_LIST_MENU;
        list->rows = coopGuardOptions(rows);
        for (i = 0; i < list->rows; i++) {
            coopWantRow(0, i, coopTradeString(TEXTS[rows[i]]), PALETTE_WHITE);
        }
        list->cursor = coopTrade.menu;
        return;
    }
    if (coopTrade.phase == COOP_TRADE_RAID_PICK) {
        /* the bosses in the list, the one under the cursor (its whole name) and its level in the box */
        s32 open[COOP_RAID_BOSSES];
        s32 count = coopRaidOpen(open);
        const u8 *names = FILE_CACHE.load(TEXT_FILE(TEXT_DIGIMON_NAMES));

        list->list = COOP_LIST_RAIDS;
        for (i = 0; i < COOP_RAID_ROWS && coopTrade.raidTop + i < count; i++) {
            coopWantRow(0, i, (const u8 *)getString((void *)names, coopRaidBosses[open[coopTrade.raidTop + i]].nameId),
                        PALETTE_WHITE);
        }
        list->rows = i;
        list->cursor = coopTrade.raidCursor - coopTrade.raidTop;
        buf[0] = 0;
        coopTradeCat(buf, sizeof buf, (const u8 *)getString((void *)names, coopRaidBosses[open[coopTrade.raidCursor]].nameId));
        coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE COOP_SPACE);
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_RAID_LEVEL));
        coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE); /* "Lv 17" */
        coopTradeNumber(num, 0, coopRaidBosses[open[coopTrade.raidCursor]].level);
        coopTradeCat(buf, sizeof buf, num);
        coopWantBox(buf, -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.kind >= 1 && coopTrade.phase >= COOP_TRADE_OPEN) {
        coopWantBox(coopTradeString(COOP_TEXT_GUARD_PREPARING), -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK && coopTrade.kind == 2) {
        /* the raid's room: "Waiting for another player... 2/4  9" */
        s32 secs = coopTrade.roomCount >= 2 ? (COOP_ROOM_WAIT - coopTrade.roomFrames + 59) / 60 : 0;

        buf[0] = 0;
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_GUARD_WAIT_OTHER));
        coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE);
        coopTradeNumber(num, 0, coopTrade.roomCount);
        coopTradeCat(buf, sizeof buf, num);
        coopTradeCat(buf, sizeof buf, coopTradeString(COOP_TEXT_RAID_OF_FOUR));
        if (secs > 0) {
            coopTradeCat(buf, sizeof buf, (const u8 *)COOP_SPACE);
            coopTradeNumber(num, 0, secs > 99 ? 99 : secs);
            coopTradeCat(buf, sizeof buf, num);
        }
        coopWantBox(buf, -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    if (coopTrade.phase == COOP_TRADE_ASK) {
        coopWantBox(coopTradeString(COOP_TEXT_GUARD_WAIT_OTHER), -1, COOP_TEXT_TRADE_LEAVE);
        return;
    }
    coopTradeWant(in);
}
