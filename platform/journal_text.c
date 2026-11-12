/* The Digilog's texts (tools/journal_steps.py): "Step ", " of ", the digits and each step's, apart from the
   co-op's (platform/team.c's native_coop_text) */
#include <stdint.h>
#include "journal_text.h"

/* As a text file of the game's language (LANGUAGE: 2 English to 6 Spanish) */
const void *native_journal_text(int language) {
    static const uint32_t *const BY_LANGUAGE[] = {JOURNAL_TEXT_ENG, JOURNAL_TEXT_ENG, JOURNAL_TEXT_ENG, JOURNAL_TEXT_FRA,
                                                  JOURNAL_TEXT_ITA, JOURNAL_TEXT_GER, JOURNAL_TEXT_SPN};

    return BY_LANGUAGE[language >= 0 && language < 7 ? language : 2];
}
