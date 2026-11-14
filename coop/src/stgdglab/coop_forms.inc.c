/* Asuka Link: the lab's chart (stgdglab/recipe_screen.c includes it) shows, on square, every form of the partner and
   what each needs, and from there on square a form's line: the forms it comes from and those it leads to. All of
   it is read from the evolution lists STFGTREP_learnDigimon follows (STFGTREP_evolutions), the same checks: so it
   stays right when the lists change. Its windows are the chart's own (the title bars, the frames and portraits, the
   name panel), with more text windows of the game's. Not built alone: it uses recipe_screen.c's definitions. */

#include "coop_text_ids.h"

const void *native_coop_text(int language);

/* STFGTREP's Evolution (include/stfgtrep.h), which can't be included with this overlay's header */
typedef struct CoopEvolution {
    s32 digimon; /* DIGIMON_DATA index + 1 */
    struct {
        s16 digimon; /* 0 for none */
        s16 level;
    } needs[2];
    s16 stat; /* 1-6 battle stats, 7 level, 8-14 resistances */
    s16 value;
} CoopEvolution;
extern CoopEvolution *STFGTREP_evolutions[8];

#define COOP_FORMS_CHART 0
#define COOP_FORMS_LIST 1
#define COOP_FORMS_LINE 2

#define COOP_FORM_MISSING 0
#define COOP_FORM_READY 1 /* meets what it needs: learnt after a battle, one a battle */
#define COOP_FORM_NEXT 2 /* the first ready one, the one the next battle gives */
#define COOP_FORM_OWNED 3

#define COOP_FORMS_MAX (44 + NATIVE_EXTRA_FORMS) /* the game's 44, BladeGarurumon and the mods' (stfgtrep/growth.c) */
#define COOP_FORMS_ROOKIE COOP_FORMS_MAX /* the partner itself, as a node of the line */
#define COOP_FORMS_ROWS 7 /* the list's, two names each */
#define COOP_FORMS_BUF 64

#define COOP_ICON_CROSS 0x1B
#define COOP_ICON_CIRCLE 0x2F
#define COOP_ICON_SQUARE 0x32
#define COOP_ICON_TRIANGLE 0x34
#define COOP_ICON_SLASH 0x0F

typedef struct CoopFormsWindows {
    LabRecipeScreenWindows base;
    TextWindow *hint; /* the chart's "square: forms" */
    TextWindow *list[COOP_FORMS_ROWS * 2];
    TextWindow *listHint;
    TextWindow *name; /* the panel: the form, how it stands, what it needs and what the partner has */
    TextWindow *status;
    TextWindow *lines[3];
    TextWindow *values[3];
    Cursor *cursor;
} CoopFormsWindows;

typedef struct CoopFormsScreen {
    LabRecipeScreen base;
    s32 mode;
    CoopEvolution list[COOP_FORMS_MAX]; /* the partner's evolutions, as STFGTREP_learnDigimon goes through them */
    s32 listCount;
    s32 count; /* the partner's forms: indices into list */
    s32 forms[COOP_FORMS_MAX];
    s32 status[COOP_FORMS_MAX + 1]; /* by evolution index; the rookie's last */
    s32 pick; /* the list's: into forms */
    s32 scroll; /* its first row */
    s32 center; /* the line's form: an evolution index or COOP_FORMS_ROOKIE */
    s32 prevs[2];
    s32 prevCount;
    s32 nexts[COOP_FORMS_MAX];
    s32 nextCount;
    s32 col; /* the line's cursor: 0 the forms before, 1 the form, 2 those after */
    s32 row;
    s32 nextScroll;
    s32 shown; /* the node the panel shows, -1 none */
    s32 blink;
    s32 blinkTime;
    u8 buf[12][COOP_FORMS_BUF];
} CoopFormsScreen;

#define COOP_FORMS(screen) ((CoopFormsScreen *)(screen))
#define COOP_FORMS_WIN(win) ((CoopFormsWindows *)(win))

static const u8 *coopFormsText(s32 n) {
    return (const u8 *)getString((s32 *)native_coop_text(LANGUAGE), n); /* a COOP_TEXT_* */
}

static void coopFormsCat(u8 *buf, const u8 *text) {
    s32 n = 0;

    while (n < COOP_FORMS_BUF - 1 && buf[n] != 0) {
        n++;
    }
    while (n < COOP_FORMS_BUF - 1 && *text != 0) {
        buf[n++] = *text++;
    }
    buf[n] = 0;
}

static void coopFormsIcon(u8 *buf, s32 icon) {
    u8 code[3] = {0x01, 0, 0};

    code[1] = icon;
    coopFormsCat(buf, code);
}

/* A number in the game's digits (0 is 0x04) */
static void coopFormsNumber(u8 *buf, s32 value) {
    u8 digits[8];
    s32 n = 7;

    digits[7] = 0;
    if (value < 0) {
        value = 0;
    }
    do {
        digits[--n] = 0x04 + value % 10;
        value /= 10;
    } while (value != 0 && n > 0);
    coopFormsCat(buf, &digits[n]);
}

#define COOP_SPACE ((const u8 *)"\x01\x01")

static s32 coopFormsDigimonId(s32 digimon) { /* an index + 1, as the lists have them */
    return DIGIMON_DATA[digimon - 1].id;
}

static const u8 *coopFormsName(s32 id) {
    return (const u8 *)getString((s32 *)FILE_CACHE.load(TEXT_FILE(TEXT_DIGIMON_NAMES)), GET_DIGIMON(id)->nameId);
}

/* The node's Digimon id */
static s32 coopFormsNodeId(LabRecipeScreen *screen, s32 node) {
    if (node == COOP_FORMS_ROOKIE) {
        return DIGIMON_DATA[screen->table].id; /* the first eight are the partners */
    }
    return coopFormsDigimonId(COOP_FORMS(screen)->list[node].digimon);
}

/* The level the partner has a Digimon (an index + 1) at, -1 without it */
static s32 coopFormsLevel(s32 partner, s32 digimon) {
    PartnerEntry entry;

    if (GAME.funcs.getPartnerEntry(partner, coopFormsDigimonId(digimon), &entry) == -1) {
        return -1;
    }
    return entry.level;
}

/* The partner's own value of an Evolution's stat, as STFGTREP_learnDigimon reads it (no equipment, no form) */
static s32 coopFormsStat(s32 partner, s32 stat) {
    PartnerStats *stats = GAME.funcs.getPartnerStats(partner);

    if (stat < 7) {
        return stats->stats[stat + 5];
    }
    if (stat == 7) {
        return stats->stats[STAT_LEVEL];
    }
    return stats->stats[stat + 4];
}

/* Whether the partner meets all an evolution needs (STFGTREP_learnDigimon's checks) */
static s32 coopFormsMeets(s32 partner, CoopEvolution *evo) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (evo->needs[i].digimon != 0 && coopFormsLevel(partner, evo->needs[i].digimon) < evo->needs[i].level) {
            return 0;
        }
    }
    return evo->stat == 0 || coopFormsStat(partner, evo->stat) >= evo->value;
}

/* The evolution index of a Digimon (an index + 1) in the partner's list, -1 if it isn't there */
static s32 coopFormsFind(LabRecipeScreen *screen, s32 digimon) {
    s32 i;

    for (i = 0; i < COOP_FORMS(screen)->listCount; i++) {
        if (COOP_FORMS(screen)->list[i].digimon == digimon) {
            return i;
        }
    }
    return -1;
}

/* The partner's evolution list as STFGTREP_learnDigimon goes through it in the port (stfgtrep/growth.c): its 44,
   then BladeGarurumon (Growlmon and Grizzmon at level 40) while it is on, then the Digimon mods' */
static void coopFormsLoadList(LabRecipeScreen *screen) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopEvolution *evo;
    const NativeMod *m;
    s32 n;

    memcpy(forms->list, STFGTREP_evolutions[screen->table], 44 * sizeof(CoopEvolution));
    forms->listCount = 44;
    if (g_bladegarurumon) {
        evo = &forms->list[forms->listCount++];
        evo->digimon = NATIVE_BLADEGARURUMON_DATA + 1;
        evo->needs[0].digimon = 14, evo->needs[0].level = 40; /* Growlmon */
        evo->needs[1].digimon = 11, evo->needs[1].level = 40; /* Grizzmon */
        evo->stat = 0, evo->value = 1;
    }
    for (n = 0; n < NATIVE_MAX_MODS; n++) {
        m = native_mod_at(n);
        if (m != NULL && m->needs[0] > 0 && m->needs[2] > 0) {
            evo = &forms->list[forms->listCount++];
            evo->digimon = m->slot + 1;
            evo->needs[0].digimon = m->needs[0], evo->needs[0].level = m->needs[1];
            evo->needs[1].digimon = m->needs[2], evo->needs[1].level = m->needs[3];
            evo->stat = 0, evo->value = 1;
        }
    }
}

/* The partner's forms and how each stands; the first ready one is the one the next battle gives */
static void coopFormsLoad(LabRecipeScreen *screen) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopEvolution *list = forms->list;
    PartnerEntry entry;
    s32 next = 0;
    s32 id;
    s32 i;

    coopFormsLoadList(screen);
    forms->count = 0;
    for (i = 0; i < forms->listCount; i++) {
        forms->status[i] = COOP_FORM_MISSING;
        if (list[i].digimon <= 0 || (id = coopFormsDigimonId(list[i].digimon)) <= 0) {
            continue; /* STFGTREP_learnDigimon skips it too */
        }
        forms->forms[forms->count++] = i;
        if (GAME.funcs.getPartnerEntry(screen->table, id, &entry) >= 0) {
            forms->status[i] = COOP_FORM_OWNED;
        } else if (coopFormsMeets(screen->table, &list[i])) {
            forms->status[i] = next ? COOP_FORM_READY : COOP_FORM_NEXT;
            next = 1;
        }
    }
    forms->status[COOP_FORMS_ROOKIE] = COOP_FORM_OWNED;
}

static s32 coopFormsPalette(s32 status) {
    switch (status) {
    case COOP_FORM_OWNED:
        return PALETTE_WHITE;
    case COOP_FORM_NEXT:
        return PALETTE_YELLOW;
    case COOP_FORM_READY:
        return PALETTE_GREEN;
    default:
        return PALETTE_GREY;
    }
}

/* A line of the panel: the circle or the cross, what is needed, and on the right what the partner has. The panel
   has room for two: with append, a second need goes after the line's first (an evolution with two forms and a stat) */
static void coopFormsLine(CoopFormsScreen *forms, CoopFormsWindows *win, s32 i, s32 met, const u8 *need, s32 needValue,
                          s32 lv, s32 have, s32 append) {
    u8 *left = forms->buf[2 + i];
    u8 *right = forms->buf[5 + i];

    if (append) {
        coopFormsCat(left, COOP_SPACE);
        coopFormsCat(left, COOP_SPACE);
        coopFormsCat(right, COOP_SPACE);
        met = met && win->values[i]->palette == PALETTE_GREEN;
    } else {
        left[0] = 0;
        right[0] = 0;
    }
    coopFormsIcon(left, met ? COOP_ICON_CIRCLE : COOP_ICON_CROSS);
    coopFormsCat(left, COOP_SPACE);
    coopFormsCat(left, need);
    coopFormsCat(left, COOP_SPACE);
    if (lv) {
        coopFormsCat(left, coopFormsText(COOP_TEXT_FORMS_LV));
    }
    coopFormsNumber(left, needValue);
    if (have < 0) {
        coopFormsCat(right, (const u8 *)"\x01\x1A"); /* "-": it hasn't the form */
    } else {
        if (lv) {
            coopFormsCat(right, coopFormsText(COOP_TEXT_FORMS_LV));
        }
        coopFormsNumber(right, have);
    }
    win->lines[i]->setString(win->lines[i], (char *)left, -1);
    win->lines[i]->setPalette(win->lines[i], PALETTE_WHITE);
    win->values[i]->setString(win->values[i], (char *)right, -1);
    win->values[i]->setPalette(win->values[i], met ? PALETTE_GREEN : PALETTE_RED);
    win->values[i]->setRightAlign(win->values[i], 1);
}

/* The panel: the node's name, how it stands, and each thing it needs, met or not */
static void coopFormsShowPanel(LabRecipeScreen *screen, CoopFormsWindows *win, s32 node) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopEvolution *evo;
    PartnerEntry entry;
    s32 status = forms->status[node];
    s32 have;
    s32 n = 0;
    s32 i;

    forms->shown = node;
    win->name->setString(win->name, (char *)coopFormsName(coopFormsNodeId(screen, node)), -1);
    forms->buf[1][0] = 0;
    if (status == COOP_FORM_OWNED) {
        GAME.funcs.getPartnerEntry(screen->table, coopFormsNodeId(screen, node), &entry);
        coopFormsCat(forms->buf[1], coopFormsText(COOP_TEXT_FORMS_OWNED));
        coopFormsCat(forms->buf[1], COOP_SPACE);
        coopFormsCat(forms->buf[1], coopFormsText(COOP_TEXT_FORMS_LV));
        coopFormsNumber(forms->buf[1], node == COOP_FORMS_ROOKIE ? coopFormsStat(screen->table, 7) : entry.level);
    } else if (status == COOP_FORM_NEXT) {
        coopFormsCat(forms->buf[1], coopFormsText(COOP_TEXT_FORMS_NEXT));
    } else if (status == COOP_FORM_READY) {
        coopFormsCat(forms->buf[1], coopFormsText(COOP_TEXT_FORMS_READY));
    }
    win->status->setString(win->status, (char *)forms->buf[1], -1);
    win->status->setPalette(win->status, status == COOP_FORM_OWNED ? PALETTE_BLUE : coopFormsPalette(status));
    win->status->setRightAlign(win->status, 1);
    if (node != COOP_FORMS_ROOKIE) {
        evo = &forms->list[node];
        for (i = 0; i < 2; i++) {
            if (evo->needs[i].digimon != 0) {
                have = coopFormsLevel(screen->table, evo->needs[i].digimon);
                /* two forms and a stat: the forms share the first line */
                if (n == 1 && evo->stat != 0) {
                    coopFormsLine(forms, win, 0, have >= evo->needs[i].level,
                                  coopFormsName(coopFormsDigimonId(evo->needs[i].digimon)), evo->needs[i].level, 1, have, 1);
                } else {
                    coopFormsLine(forms, win, n++, have >= evo->needs[i].level,
                                  coopFormsName(coopFormsDigimonId(evo->needs[i].digimon)), evo->needs[i].level, 1, have, 0);
                }
            }
        }
        if (evo->stat >= 1 && evo->stat <= 14) {
            have = coopFormsStat(screen->table, evo->stat);
            coopFormsLine(forms, win, n++, have >= evo->value, coopFormsText(COOP_TEXT_FORMS_STAT_1 + evo->stat - 1),
                          evo->value, 0, have, 0);
        }
        if (n == 0) {
            win->lines[n++]->setString(win->lines[0], (char *)coopFormsText(COOP_TEXT_FORMS_NONE), -1);
            win->values[0]->setVisible(win->values[0], 0);
        }
    }
    for (; n < 3; n++) {
        win->lines[n]->setVisible(win->lines[n], 0);
        win->values[n]->setVisible(win->values[n], 0);
    }
}

static void coopFormsHidePanel(CoopFormsWindows *win) {
    s32 i;

    win->name->setVisible(win->name, 0);
    win->status->setVisible(win->status, 0);
    for (i = 0; i < 3; i++) {
        win->lines[i]->setVisible(win->lines[i], 0);
        win->values[i]->setVisible(win->values[i], 0);
    }
}

/* A hint of buttons: icon and word, then a second pair (0: none) */
static void coopFormsHint(u8 *buf, s32 icon, s32 word, s32 icon2, s32 word2) {
    buf[0] = 0;
    coopFormsIcon(buf, icon);
    coopFormsCat(buf, coopFormsText(word));
    if (icon2 != 0) {
        coopFormsCat(buf, COOP_SPACE);
        coopFormsCat(buf, COOP_SPACE);
        coopFormsIcon(buf, icon2);
        coopFormsCat(buf, coopFormsText(word2));
    }
}

/* Creates the windows the chart lacks (recipe_screen.c's TASK_INIT, after its own) */
static void coopFormsInit(LabRecipeScreen *screen, LabRecipeScreenWindows *base) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopFormsWindows *win = COOP_FORMS_WIN(base);
    s32 i;

    forms->mode = COOP_FORMS_CHART;
    forms->shown = -1;
    win->hint = createTextWindow(screen->layer, 1, 0x84, 0xCA);
    coopFormsHint(forms->buf[0], COOP_ICON_SQUARE, COOP_TEXT_FORMS_WORD, 0, 0);
    win->hint->setString(win->hint, (char *)forms->buf[0], -1);
    win->hint->setVisible(win->hint, 0);
    for (i = 0; i < COOP_FORMS_ROWS * 2; i++) {
        win->list[i] = createTextWindow(screen->layer, 1, (i & 1) ? 0xA8 : 0x24, (i >> 1) * 0xE + 0x34);
        win->list[i]->setVisible(win->list[i], 0);
    }
    win->listHint = createTextWindow(screen->layer, 1, 0x24, 0x98);
    win->listHint->setVisible(win->listHint, 0);
    win->name = createTextWindow(screen->layer, 1, 0x3A, 0xAD);
    win->status = createTextWindow(screen->layer, 1, 0x12C, 0xAD);
    for (i = 0; i < 3; i++) {
        win->lines[i] = createTextWindow(screen->layer, 1, 0x3A, i * 0xE + 0xBC);
        win->values[i] = createTextWindow(screen->layer, 1, 0x12C, i * 0xE + 0xBC);
    }
    win->name->setDepth(win->name, 0);
    win->status->setDepth(win->status, 0);
    for (i = 0; i < 3; i++) {
        win->lines[i]->setDepth(win->lines[i], 0);
        win->values[i]->setDepth(win->values[i], 0);
    }
    coopFormsHidePanel(win);
    win->cursor = createCursor(screen->layer, 0, 0x17, 0x33);
    win->cursor->setVisible(win->cursor, 0);
}

/* The list's page: the names in their colours, the cursor, the count of forms learnt in the title bar */
static void coopFormsShowList(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    s32 i;
    s32 n;

    for (i = 0; i < COOP_FORMS_ROWS * 2; i++) {
        n = forms->scroll * 2 + i;
        if (n < forms->count) {
            win->list[i]->setString(win->list[i], (char *)coopFormsName(coopFormsNodeId(screen, forms->forms[n])), -1);
            win->list[i]->setPalette(win->list[i], coopFormsPalette(forms->status[forms->forms[n]]));
        } else {
            win->list[i]->setVisible(win->list[i], 0);
        }
    }
    n = forms->pick - forms->scroll * 2;
    win->cursor->setPos(win->cursor, (n & 1) ? 0x9B : 0x17, (n >> 1) * 0xE + 0x33);
    win->cursor->setVisible(win->cursor, 1);
    coopFormsShowPanel(screen, win, forms->forms[forms->pick]);
}

/* The title bars: the partner's forms and how many it has, or the line's form */
static void coopFormsShowTitle(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    s32 owned = 0;
    s32 i;

    forms->buf[0][0] = 0;
    if (forms->mode == COOP_FORMS_LIST) {
        coopFormsCat(forms->buf[0], coopFormsText(COOP_TEXT_FORMS_TITLE_PRE));
        coopFormsCat(forms->buf[0], (const u8 *)GAME.funcs.getPartnerStats(screen->table)->name);
        coopFormsCat(forms->buf[0], coopFormsText(COOP_TEXT_FORMS_TITLE_POST));
        for (i = 0; i < forms->count; i++) {
            owned += forms->status[forms->forms[i]] == COOP_FORM_OWNED;
        }
        win->base.rowNumber->setNumber(win->base.rowNumber, 0, owned);
        win->base.rowNumber->setRightAlign(win->base.rowNumber, 1);
        forms->buf[11][0] = 0;
        coopFormsIcon(forms->buf[11], COOP_ICON_SLASH);
        coopFormsNumber(forms->buf[11], forms->count);
        win->base.rowLabel->setString(win->base.rowLabel, (char *)forms->buf[11], -1);
    } else {
        coopFormsCat(forms->buf[0], coopFormsText(COOP_TEXT_FORMS_LINE_TITLE));
        coopFormsCat(forms->buf[0], coopFormsName(coopFormsNodeId(screen, forms->center)));
        win->base.rowNumber->setVisible(win->base.rowNumber, 0);
        win->base.rowLabel->setVisible(win->base.rowLabel, 0);
    }
    win->base.title->setString(win->base.title, (char *)forms->buf[0], -1);
}

/* The line of a node: the forms it needs (the rookie when none), and those that need it */
static void coopFormsCenter(LabRecipeScreen *screen, s32 node) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopEvolution *list = forms->list;
    CoopEvolution *evo;
    s32 digimon = node == COOP_FORMS_ROOKIE ? -1 : list[node].digimon;
    s32 found;
    s32 i;
    s32 j;
    s32 n;

    forms->center = node;
    forms->prevCount = 0;
    if (node != COOP_FORMS_ROOKIE) {
        for (i = 0; i < 2; i++) {
            if (list[node].needs[i].digimon != 0 && (found = coopFormsFind(screen, list[node].needs[i].digimon)) >= 0) {
                forms->prevs[forms->prevCount++] = found;
            }
        }
        if (forms->prevCount == 0) {
            forms->prevs[forms->prevCount++] = COOP_FORMS_ROOKIE;
        }
    }
    forms->nextCount = 0;
    for (n = 0; n < forms->count; n++) {
        i = forms->forms[n];
        evo = &list[i];
        found = 0;
        for (j = 0; j < 2; j++) {
            if (evo->needs[j].digimon != 0 && evo->needs[j].digimon == digimon) {
                found = 1;
            }
        }
        if (node == COOP_FORMS_ROOKIE) {
            found = evo->needs[0].digimon == 0 && evo->needs[1].digimon == 0;
        }
        if (found) {
            forms->nexts[forms->nextCount++] = i;
        }
    }
    forms->col = 1;
    forms->row = 0;
    forms->nextScroll = 0;
}

/* The node under the line's cursor */
static s32 coopFormsLineNode(CoopFormsScreen *forms) {
    if (forms->col == 0) {
        return forms->prevs[forms->row];
    }
    if (forms->col == 2) {
        return forms->nexts[forms->nextScroll + forms->row];
    }
    return forms->center;
}

/* Where a column's row goes: centred on the form's */
static s32 coopFormsRowY(s32 count, s32 row) {
    if (count >= 3) {
        return row * 0x28 + 0x32;
    }
    if (count == 2) {
        return row * 0x28 + 0x46;
    }
    return 0x5A;
}

static s32 coopFormsColX(s32 col) {
    return col * 0x58 + 0x30;
}

static void coopFormsListMode(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);

    forms->mode = COOP_FORMS_LIST;
    coopFormsHint(forms->buf[10], COOP_ICON_SQUARE, COOP_TEXT_FORMS_LINE, COOP_ICON_TRIANGLE, COOP_TEXT_FORMS_BACK);
    win->listHint->setString(win->listHint, (char *)forms->buf[10], -1);
    coopFormsShowTitle(screen, win);
    coopFormsShowList(screen, win);
}

/* Square on the chart: the list opens on the form under the chart's cursor */
static void coopFormsOpen(LabRecipeScreen *screen, LabRecipeScreenWindows *base) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopFormsWindows *win = COOP_FORMS_WIN(base);
    s32 id = screen->found[screen->row][screen->slot][screen->col];
    s32 i;

    SOUND.playSound(SOUND_MENU_CONFIRM);
    coopFormsLoad(screen);
    if (forms->count == 0) {
        return;
    }
    forms->pick = 0;
    for (i = 0; i < forms->count; i++) {
        if (id > 0 && coopFormsNodeId(screen, forms->forms[i]) == id) {
            forms->pick = i;
        }
    }
    forms->scroll = forms->pick / 2 - COOP_FORMS_ROWS / 2;
    if (forms->scroll > (forms->count + 1) / 2 - COOP_FORMS_ROWS) {
        forms->scroll = (forms->count + 1) / 2 - COOP_FORMS_ROWS;
    }
    if (forms->scroll < 0) {
        forms->scroll = 0;
    }
    base->prev->setVisible(base->prev, 0);
    base->next->setVisible(base->next, 0);
    win->hint->setVisible(win->hint, 0);
    coopFormsListMode(screen, win);
}

/* Back to the chart: its title, page number and arrows as runRecipeRow leaves them */
static void coopFormsClose(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    s32 i;

    forms->mode = COOP_FORMS_CHART;
    for (i = 0; i < COOP_FORMS_ROWS * 2; i++) {
        win->list[i]->setVisible(win->list[i], 0);
    }
    win->listHint->setVisible(win->listHint, 0);
    win->cursor->setVisible(win->cursor, 0);
    coopFormsHidePanel(win);
    win->base.title->setString(win->base.title, FILE_CACHE.load(TEXT_FILE(TEXT_DIGI_LAB)), 0x27);
    win->base.title->setSubString(win->base.title, GAME.funcs.getPartnerStats(screen->table), -1, 1);
    win->base.rowNumber->setNumber(win->base.rowNumber, 0, screen->row + 1);
    win->base.rowNumber->setRightAlign(win->base.rowNumber, 1);
    win->base.rowLabel->setString(win->base.rowLabel, FILE_CACHE.load(TEXT_FILE(TEXT_DIGI_LAB)), 0x28);
    win->base.prev->setVisible(win->base.prev, screen->row != 0);
    win->base.next->setVisible(win->base.next, screen->row != screen->rowCount - 1);
}

/* The list: the cursor goes through the names (two a row, scrolling), square opens the form's line, triangle goes
   back to the chart */
static void coopFormsRunList(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    s32 old = forms->pick;

    if (PAD_PRESSED(PAD_TRIANGLE)) {
        SOUND.playSound(SOUND_MENU_CANCEL);
        coopFormsClose(screen, win);
        return;
    }
    if (PAD_PRESSED(PAD_SQUARE) || PAD_PRESSED(PAD_CROSS)) {
        SOUND.playSound(SOUND_MENU_CONFIRM);
        forms->mode = COOP_FORMS_LINE;
        for (old = 0; old < COOP_FORMS_ROWS * 2; old++) {
            win->list[old]->setVisible(win->list[old], 0);
        }
        win->cursor->setVisible(win->cursor, 0);
        coopFormsCenter(screen, forms->forms[forms->pick]);
        coopFormsHint(forms->buf[10], COOP_ICON_CROSS, COOP_TEXT_FORMS_LINE, COOP_ICON_TRIANGLE, COOP_TEXT_FORMS_BACK);
        win->listHint->setString(win->listHint, (char *)forms->buf[10], -1);
        coopFormsShowTitle(screen, win);
        coopFormsShowPanel(screen, win, forms->center);
        return;
    }
    if (PAD_PRESSED(PAD_UP) || PAD_REPEATED(PAD_UP)) {
        if (forms->pick >= 2) {
            forms->pick -= 2;
        }
    } else if (PAD_PRESSED(PAD_DOWN) || PAD_REPEATED(PAD_DOWN)) {
        if (forms->pick + 2 < forms->count) {
            forms->pick += 2;
        } else if ((forms->pick & 1) && forms->pick + 1 < forms->count) {
            forms->pick += 1; /* the last row has only the left name */
        }
    } else if (PAD_PRESSED(PAD_LEFT) || PAD_REPEATED(PAD_LEFT)) {
        if (forms->pick & 1) {
            forms->pick--;
        }
    } else if (PAD_PRESSED(PAD_RIGHT) || PAD_REPEATED(PAD_RIGHT)) {
        if (!(forms->pick & 1) && forms->pick + 1 < forms->count) {
            forms->pick++;
        }
    } else if (PAD_PRESSED(PAD_L1) || PAD_REPEATED(PAD_L1)) {
        forms->pick -= COOP_FORMS_ROWS * 2;
        if (forms->pick < 0) {
            forms->pick = old & 1;
        }
    } else if (PAD_PRESSED(PAD_R1) || PAD_REPEATED(PAD_R1)) {
        forms->pick += COOP_FORMS_ROWS * 2;
        if (forms->pick >= forms->count) {
            forms->pick = forms->count - 1;
        }
    }
    if (forms->pick != old) {
        SOUND.playSound(SOUND_MENU_MOVE);
        if (forms->pick / 2 < forms->scroll) {
            forms->scroll = forms->pick / 2;
        } else if (forms->pick / 2 >= forms->scroll + COOP_FORMS_ROWS) {
            forms->scroll = forms->pick / 2 - COOP_FORMS_ROWS + 1;
        }
        coopFormsShowList(screen, win);
    }
}

/* The line: the cursor goes through the forms before, the form and those after; cross on another makes it the
   line's form, triangle goes back to the list (on the line's form) */
static void coopFormsRunLine(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    s32 oldCol = forms->col;
    s32 oldRow = forms->row;
    s32 oldScroll = forms->nextScroll;
    s32 node;
    s32 i;

    if (PAD_PRESSED(PAD_TRIANGLE) || PAD_PRESSED(PAD_SQUARE)) {
        SOUND.playSound(SOUND_MENU_CANCEL);
        for (i = 0; i < forms->count; i++) {
            if (forms->forms[i] == forms->center) {
                forms->pick = i;
                if (i / 2 < forms->scroll) {
                    forms->scroll = i / 2;
                } else if (i / 2 >= forms->scroll + COOP_FORMS_ROWS) {
                    forms->scroll = i / 2 - COOP_FORMS_ROWS + 1;
                }
            }
        }
        coopFormsListMode(screen, win);
        return;
    }
    if (PAD_PRESSED(PAD_CROSS)) {
        node = coopFormsLineNode(forms);
        if (forms->col != 1) {
            SOUND.playSound(SOUND_MENU_CONFIRM);
            coopFormsCenter(screen, node);
            coopFormsShowTitle(screen, win);
            coopFormsShowPanel(screen, win, forms->center);
        }
        return;
    }
    if (PAD_PRESSED(PAD_LEFT) || PAD_REPEATED(PAD_LEFT)) {
        if (forms->col > 0 && (forms->col == 2 || forms->prevCount > 0)) {
            forms->col--;
            forms->row = 0;
        }
    } else if (PAD_PRESSED(PAD_RIGHT) || PAD_REPEATED(PAD_RIGHT)) {
        if (forms->col < 2 && (forms->col == 0 || forms->nextCount > 0)) {
            forms->col++;
            forms->row = 0;
        }
    } else if (PAD_PRESSED(PAD_UP) || PAD_REPEATED(PAD_UP)) {
        if (forms->row > 0) {
            forms->row--;
        } else if (forms->col == 2 && forms->nextScroll > 0) {
            forms->nextScroll--;
        }
    } else if (PAD_PRESSED(PAD_DOWN) || PAD_REPEATED(PAD_DOWN)) {
        if (forms->col == 0 && forms->row + 1 < forms->prevCount) {
            forms->row++;
        } else if (forms->col == 2) {
            if (forms->row < 2 && forms->nextScroll + forms->row + 1 < forms->nextCount) {
                forms->row++;
            } else if (forms->nextScroll + 3 < forms->nextCount) {
                forms->nextScroll++;
            }
        }
    }
    if (oldCol != forms->col || oldRow != forms->row || oldScroll != forms->nextScroll) {
        SOUND.playSound(SOUND_MENU_MOVE);
        coopFormsShowPanel(screen, win, coopFormsLineNode(forms));
    }
}

/* recipe_screen.c's substate 3: the chart's row (with square for the list), or ours */
static void coopFormsRun(LabRecipeScreen *screen, LabRecipeScreenWindows *base) {
    CoopFormsScreen *forms = COOP_FORMS(screen);

    if (forms->mode == COOP_FORMS_LIST) {
        coopFormsRunList(screen, COOP_FORMS_WIN(base));
    } else if (forms->mode == COOP_FORMS_LINE) {
        coopFormsRunLine(screen, COOP_FORMS_WIN(base));
    }
}

/* Draws ours instead of the chart: the title bars, the line's nodes or the list's scroll arrows, and the panel with
   the portrait of the form it shows */
static void coopFormsDrawOurs(LabRecipeScreen *screen, CoopFormsWindows *win) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    SpriteDrawer sprite;
    s32 i;
    s32 n;
    s32 frame;
    /* the portraits go on the layer before the frames: two drawers, as STGDGLAB_drawRecipeScreen, the second made
       after the frames are drawn (initSpriteDrawer sets the game's own) */
    s32 nodes[8][4]; /* node, x, y, picked */
    CVECTOR normal = {0x80, 0x80, 0x80, 0};
    CVECTOR dark = {0x38, 0x38, 0x48, 0};
    s32 nodeCount = 0;

    if (GFX.funcs.getTime() - screen->time >= 8) {
        screen->time = GFX.funcs.getTime();
        if (++screen->clutRow >= 4) {
            screen->clutRow = 0;
        }
        forms->blink = 1 - forms->blink;
    }
    if (forms->mode == COOP_FORMS_LINE) {
        for (i = 0; i < forms->prevCount; i++) {
            nodes[nodeCount][0] = forms->prevs[i];
            nodes[nodeCount][1] = coopFormsColX(0);
            nodes[nodeCount][2] = coopFormsRowY(forms->prevCount, i);
            nodes[nodeCount++][3] = forms->col == 0 && forms->row == i;
        }
        nodes[nodeCount][0] = forms->center;
        nodes[nodeCount][1] = coopFormsColX(1);
        nodes[nodeCount][2] = 0x5A;
        nodes[nodeCount++][3] = forms->col == 1;
        n = forms->nextCount - forms->nextScroll;
        for (i = 0; i < 3 && i < n; i++) {
            nodes[nodeCount][0] = forms->nexts[forms->nextScroll + i];
            nodes[nodeCount][1] = coopFormsColX(2);
            nodes[nodeCount][2] = coopFormsRowY(forms->nextCount, i);
            nodes[nodeCount++][3] = forms->col == 2 && forms->row == i;
        }
    }
    initSpriteDrawer(&sprite);
    sprite.setTexture(0x280, 0x100);
    sprite.setLayerId(screen->layer, screen->depth);
    for (i = 0; i < nodeCount; i++) {
        if (nodes[i][3]) {
            sprite.setClutRow(screen->clutRow);
            sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x47, nodes[i][1], nodes[i][2]);
        }
    }
    for (i = 0; i < nodeCount; i++) {
        n = forms->status[nodes[i][0]];
        sprite.setClutRow(n == COOP_FORM_NEXT ? 2 : n == COOP_FORM_READY ? 1 : 0);
        sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), n == COOP_FORM_MISSING ? 0x40 : 0x3F, nodes[i][1],
                    nodes[i][2]);
    }
    sprite.setClutRow(0);
    if (forms->blink) {
        if (forms->mode == COOP_FORMS_LIST) {
            if (forms->scroll > 0) {
                sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x34, 0x12A, 0x2D);
            }
            if (forms->scroll + COOP_FORMS_ROWS < (forms->count + 1) / 2) {
                sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x35, 0x12A, 0x88);
            }
        } else {
            if (forms->nextScroll > 0) {
                sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x34, 0x118, 0x22);
            }
            if (forms->nextScroll + 3 < forms->nextCount) {
                sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x35, 0x118, 0x8C);
            }
        }
    }
    sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x44, 0, 0x11);
    if (forms->mode == COOP_FORMS_LIST) {
        sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x43, 0xF0, 0x11); /* the count's bar */
    }
    initSpriteDrawer(&sprite);
    sprite.setTexture(0x140, 0x100);
    sprite.setAltClut(0x280, 0);
    sprite.setLayerId(screen->layer, screen->depth - 1);
    for (i = 0; i < nodeCount; i++) {
        frame = STGDGLAB_data.funcs.getSprite(coopFormsNodeId(screen, nodes[i][0]));
        if (frame != -1) {
            sprite.setColor(forms->status[nodes[i][0]] == COOP_FORM_MISSING ? &dark : &normal); /* not yet: dimmed */
            sprite.draw(FILE_CACHE.getEntry((FILE_LAB_SPRITES << 16) | 1), frame, nodes[i][1], nodes[i][2]);
        }
    }
    sprite.setColor(&normal);
    if (forms->shown >= 0) {
        frame = STGDGLAB_data.funcs.getSprite(coopFormsNodeId(screen, forms->shown));
        if (frame != -1) {
            sprite.draw(FILE_CACHE.getEntry((FILE_LAB_SPRITES << 16) | 1), frame, 0x14, 0xAF);
        }
    }
    sprite.setTexture(0x280, 0x100);
    sprite.draw(FILE_CACHE.getEntry(FILE_LAB_SPRITES << 16), 0x42, 0, 0xA8);
}

/* recipe_screen.c's drawing while it runs: the chart (with the square hint while its row waits for a button), or
   ours */
static void coopFormsDraw(LabRecipeScreen *screen, LabRecipeScreenWindows *base) {
    CoopFormsScreen *forms = COOP_FORMS(screen);
    CoopFormsWindows *win = COOP_FORMS_WIN(base);

    if (forms->mode == COOP_FORMS_CHART) {
        win->hint->setVisible(win->hint, screen->state == TASK_RUN && screen->substate == 3);
        STGDGLAB_drawRecipeScreen(screen, base);
    } else {
        coopFormsDrawOurs(screen, win);
    }
}
