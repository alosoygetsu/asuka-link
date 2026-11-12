/* Co-op on the field (fieldstg/field.c includes it, after the parts before it): the other players' actors on the map. Not built alone:
   it uses what field.c and the earlier parts define. */

/*
 * Co-op (platform/coop.c, coop = 1 in native.ini): the other players, up to
 * 3 (COOP_GHOSTS), each an actor that looks like the player and stands where
 * that player is (coop_peers; without it, the local player's own state of a
 * moment ago, to test alone).
 *
 * Their frames need a box in VRAM that nothing else of the field uses. Once
 * the field has run a moment (its files and every actor's first frame
 * loaded), a blank box is looked for in the player's texture page, away from
 * the slots of the party and the map's characters, and else in another page
 * right of the frame buffers. Each takes a copy of the player's CLUT under
 * its frames, its colours turned so the players tell each other apart.
 */
#define COOP_GHOSTS 3
/* the ghosts' characters: the player's look (2) under ids nothing looks for, other player n's 0x7C02 + n * 0x100
   (actor_control.c's animateActor knows them) */
#define COOP_GHOST_KEY1(n) (0x7C02 + (n) * 0x100)
#define COOP_GHOST_KIND 0x10 /* not the player (0), a partner (2, 4, 8) nor a map character (1):
                                nothing bumps into it or talks to it */
#define COOP_SLOT_W 16 /* the box of a frame, in VRAM words: 64 pixels at 4 bits */
#define COOP_SLOT_H 64
#define COOP_WAIT 30 /* frames of the field before the ghosts are made */

static Actor *coopGhost[COOP_GHOSTS];
static ActorImage coopGhostImage[COOP_GHOSTS];
static s32 coopWait;
static s32 coopSeen[COOP_GHOSTS]; /* drawn on this map yet: coop.log says it once */
/* each ghost's look (actor_control.c draws it): the model (Actor.key1, 0 Junpei) and tone, tone -1 none (Junior in
   the player's colour) */
s32 coopGhostModel[COOP_GHOSTS];
s32 coopGhostTone[COOP_GHOSTS] = {-1, -1, -1};
s32 coopPlayerLook(s32 *model, s32 *tone); /* actor_control.c: 1 when the player has the creator's look */
static s32 coopTried;

static void coopReserveSlot(s32 image) {
    ActorImage *slot = &FIELDSTG_state.images.actors[image + 2];

    native_coop_vram_reserve(slot->imageX, slot->imageY, COOP_SLOT_W, COOP_SLOT_H);
    native_coop_vram_reserve(slot->clutX, slot->clutY, 16, 1);
}

void coopSynthReset(void); /* actor_control.c: the models' made sets */
static void coopTradeReset(void); /* a new field: no trade */

static s32 coopRaidGo; /* the raid started: coopRaidGoPacket goes out (till the field starts again) */

/* As the field starts: no ghosts yet, and no slots kept */
static void coopFieldStart(void) {
    s32 i;

    coopRaidGo = 0;
    coopSynthReset();
    for (i = 0; i < COOP_GHOSTS; i++) {
        coopGhost[i] = NULL;
        coopSeen[i] = 0;
        coopGhostModel[i] = 0;
        coopGhostTone[i] = -1;
    }
    coopWait = 0;
    coopTried = 0;
    native_coop_vram_reset();
    coopTradeReset();
}

/* Makes ghost n in a free place of the field's characters (FieldChildren.npcs): a task only runs as a child of one
   that runs. The slots of the party and the map's characters are kept already (coopCreateGhosts) */
static void coopCreateGhost(FieldChildren *children, Actor *player, s32 n) {
    ActorImage *mine = player->image;
    ActorImage *image = &coopGhostImage[n];
    Point page;
    s32 slot;
    s32 x;
    s32 y;

    for (slot = 0; slot < 15 && children->npcs[slot] != NULL; slot++) {
    }
    if (slot == 15) {
        native_coop_note("map %X has 15 characters already: player %d isn't shown here", GAME.funcs.getMode(), n + 2);
        return;
    }
    /* the frames and, under them at a multiple of 16 as a CLUT's x must be, the CLUT: in the player's page, or in
       another */
    page.x = mine->x;
    page.y = mine->y;
    if (!native_coop_find_vram(mine->x, mine->y, COOP_SLOT_W * 2, COOP_SLOT_H + 1, &x, &y) &&
        !native_coop_find_vram_any(COOP_SLOT_W * 2, COOP_SLOT_H + 1, &x, &y, &page.x, &page.y)) {
        native_coop_note("map %X has no room in VRAM for player %d: not shown here", GAME.funcs.getMode(), n + 2);
        native_coop_vram_dump(mine->x, mine->y);
        return;
    }
    native_coop_vram_reserve(x, y, COOP_SLOT_W * 2, COOP_SLOT_H + 1);
    native_coop_vram_mine(x, y, COOP_SLOT_W * 2, COOP_SLOT_H + 1);
    *image = *mine;
    image->x = page.x;
    image->y = page.y;
    image->clutX = (x + 15) & ~15;
    image->clutY = y + COOP_SLOT_H;
    native_coop_vram_copy(mine->clutX, mine->clutY, image->clutX, image->clutY, 16, 1);
    native_coop_vram_tint(image->clutX, image->clutY, n);
    image->imageX = x;
    image->imageY = y;
    image->u = (x - image->x) * 4;
    image->v = y - image->y;
    coopGhost[n] = children->npcs[slot] = FIELDSTG_createActor(FIELD_CHARACTER_PLAYER, COOP_GHOST_KIND, 0, NULL);
    coopGhost[n]->key1 = COOP_GHOST_KEY1(n);
    coopGhost[n]->image = image;
    native_log("coop: mode %X: player %d's frames at VRAM (%d, %d), page (%d, %d)", GAME.funcs.getMode(), n + 2, x, y,
               image->x, image->y);
}

static void coopCreateGhosts(FieldChildren *children, Actor *player) {
    FieldActorEntry **list;
    s32 i;
    s32 n = native_coop_peer_slots();

    if (n == 0) {
        return;
    }
    if (player->key1 != FIELD_CHARACTER_PLAYER) {
        native_coop_note("map %X: you are character %X here, the others aren't shown", GAME.funcs.getMode(), player->key1);
        return;
    }
    native_coop_vram_reclaim(); /* the last map's players' boxes, when nothing came over them */
    for (i = -2; i < 4; i++) {
        coopReserveSlot(i);
    }
    for (list = FIELDSTG_state.actors; list != NULL && *list != NULL; list++) {
        coopReserveSlot((*list)->image);
        if (g_coop_debug) {
            native_log("coop: map character %X at (%d, %d)", (*list)->id, (*list)->x, (*list)->y);
        }
    }
    for (i = 0; i < n && i < COOP_GHOSTS; i++) {
        coopCreateGhost(children, player, i);
    }
}
