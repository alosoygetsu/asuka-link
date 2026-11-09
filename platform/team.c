/* The free team: a new game's three partners picked one by one among the eight (coop/patches stplnmet/choice.c), in
   place of the starter packs. GAME.partySet keeps them: the game's packs 0-2 as they were (a team that is one of them
   gets its own lobby scene), the port's extra packs 3-5 for the saves made with them, and any other team as
   NATIVE_TEAM_BASE + a + b * 8 + c * 64. The lobby's scene of a team that isn't a game pack is Powerful's (native.h's
   NATIVE_PACK_SCENE), with each partner's look and greeting from the scenes it has (fieldstg's state.c, speech.c). */
#include "platform.h"
#include "../include/native.h"
#include "coop_text.h"

int native_team_partner(int set, int i) {
    if (set >= NATIVE_TEAM_BASE) {
        return ((set - NATIVE_TEAM_BASE) >> (3 * i)) & 7;
    }
    if (set >= 3 && set < 6) {
        return NATIVE_EXTRA_PACKS[set - 3][i];
    }
    return -1;
}

int native_team_set(const int team[3]) {
    return NATIVE_TEAM_BASE + team[0] + team[1] * 8 + team[2] * 64;
}

/* The lobby: each partner's look (FIELDSTG_fileEntries) and greeting (the lobby dialogue's sub and line, or 0xFF
   and the port's own text's line), as the extra packs had them. By partner: 0 Kotemon, 1 Kumamon, 2 Monmon,
   3 Agumon, 4 Veemon, 5 Guilmon, 6 Renamon, 7 Patamon */
int native_team_look(int partner) {
    static const unsigned char LOOKS[8] = {0xB6, 0xB4, 0xB5, 0x22, 0x23, 0x40, 0x41, 0x42};

    return LOOKS[partner & 7];
}

void native_team_greeting(int partner, int *sub, int *line) {
    static const unsigned char GREETINGS[8][2] = {{3, 15}, {5, 15}, {4, 15}, {4, 16}, {0xFF, 11}, {5, 16}, {4, 17}, {3, 17}};

    *sub = GREETINGS[partner & 7][0];
    *line = GREETINGS[partner & 7][1];
}

/* The co-op build's own texts (tools/coop_text.py), as a text file of the game's language (LANGUAGE: 2 English to
   6 Spanish) */
const void *native_coop_text(int language) {
    static const uint32_t *const BY_LANGUAGE[] = {COOP_TEXT_ENG, COOP_TEXT_ENG, COOP_TEXT_ENG, COOP_TEXT_FRA,
                                                  COOP_TEXT_ITA, COOP_TEXT_GER, COOP_TEXT_SPN};

    return BY_LANGUAGE[language >= 0 && language < 7 ? language : 2];
}
