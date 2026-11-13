/* Our tests' steps on the field (DW3_STEPS, platform/pad.c; scripts/probar.ps1 -Steps): not for players */

/* Whether the field leaves the player to the pad: no event, warp, battle, talk, inn or the area's banner (shown as
   the field comes in) has them, as battles and triggers test it (battle.c, trigger.c) */
int native_test_field_free(void) {
    return FIELDSTG_state.busy == 0 && FIELDSTG_state.acting == 0 && FIELDSTG_state.innOpen == 0 &&
           FIELDSTG_state.battleStarting == 0 && FIELDSTG_state.bannerShown == 0;
}

/* Talks to the map's character (Actor.key1, dw3.py charlas' "actor N") wherever the player stands, as cross facing it
   does (FIELDSTG_talkToActorAt): its first talk whose conditions hold. 0 when it isn't on the map */
int native_test_talk(int character) {
    Actor *player = TASK_REGISTRY.funcs.find(FIELD_TASK_ACTOR, -1, 0);
    Actor *target = TASK_REGISTRY.funcs.find(FIELD_TASK_ACTOR, character, -1);
    Point pos;

    if (player == NULL || target == NULL || target == player) {
        return 0;
    }
    pos = target->tile;
    return FIELDSTG_talkToActorAt(player, &pos);
}
