"""The co-op build's own texts, in the game's text encoding (tools/game_text.py), into platform/coop_text.h: one text
file per language. Each function has its block, and each string its name: include/coop_text_ids.h has a
COOP_TEXT_<BLOCK>_<NAME> for each, its index, so no code counts strings, and a block can grow without moving another's.
Index 0 is "" (COOP_TEXT_EMPTY). The Digilog's steps have their own texts (tools/journal_steps.py).
Run from anywhere: python native/tools/coop_text.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import game_text  # noqa: E402

# (block, its strings' names, {language: its strings}):
# TEAM the partners' choice at a new game (stplnmet), LOOK the look's page, TRADE the card trade's window,
# GUARD the Guardromon's menu and its waits (fieldstg/field.c), RAID the raid's list, room and battle (wfightmn/menu.c),
# MENU the field menu's rows of our own (main/menu/field_menu.c: OPTIONS opens platform/menu.c's options)
# SHOP the item shop's yes in Spanish (stitshop/trade.c)
BLOCKS = [
    ('TEAM', ['TITLE', 'HELP', 'YOURS', 'INTRO', 'MARK', 'MARK_2', 'MARK_3', 'SEPARATOR', 'NONE', 'ACCEPT'], {
        'eng': ['Choose your Partners', 'Pick 3 partners; pick one again\nto drop it. Then Accept.', 'Your Team', 'Please choose three partner\nDigimon, any three you like.', '1', '2', '3', ', ', 'No partners yet', 'Accept'],
        'fra': ['Choisis tes partenaires', 'Choisis 3 partenaires; le choisir\nà nouveau le retire. Puis Valider.', 'Ton équipe', 'Choisis trois Digimon\npartenaires, ceux que tu veux.', '1', '2', '3', ', ', 'Aucun partenaire', 'Valider'],
        'ita': ['Scegli i tuoi partner', 'Scegli 3 partner; sceglierlo di\nnuovo lo toglie. Poi Conferma.', 'La tua squadra', 'Scegli tre Digimon partner,\nquelli che preferisci.', '1', '2', '3', ', ', 'Nessun partner', 'Conferma'],
        'ger': ['Wähle deine Partner', 'Wähle 3 Partner; erneut wählen\nentfernt ihn. Dann Weiter.', 'Dein Team', 'Wähle drei Partner-Digimon,\nganz nach deinem Geschmack.', '1', '2', '3', ', ', 'Noch keine Partner', 'Weiter'],
        'spn': ['Elige tus compañeros', 'Elige 3 compañeros; elegir otra\nvez lo quita. Luego, Aceptar.', 'Tu equipo', 'Elige a tres Digimon\ncompañeros, los que quieras.', '1', '2', '3', ', ', 'Sin compañeros aún', 'Aceptar'],
    }),
    ('LOOK', ['TITLE', 'HELP', 'NAME', 'NAME_2', 'NAME_3'], {
        'eng': ['Choose your Look', 'Pick who you are and a colour.', 'Junior', 'Boy', 'Girl'],
        'fra': ['Choisis ton look', 'Choisis qui tu es et une couleur.', 'Junior', 'Garçon', 'Fille'],
        'ita': ['Scegli il tuo aspetto', 'Scegli chi sei e un colore.', 'Junior', 'Ragazzo', 'Ragazza'],
        'ger': ['Wähle dein Aussehen', 'Wähle, wer du bist, und eine Farbe.', 'Junior', 'Junge', 'Mädchen'],
        'spn': ['Elige tu personaje', 'Elige quién eres y un color.', 'Junior', 'Chico', 'Chica'],
    }),
    ('TRADE', ['TITLE', 'YOURS', 'THEIRS', 'SPARE', 'NO_SPARE', 'WANTS', 'ACCEPT', 'LEAVE', 'WAITING', 'READY', 'CONFIRM', 'DONE', 'CANCELLED', 'DECLINED', 'NO_ROOM', 'AMOUNT', 'CONFIRMED', 'NOTHING', 'FULL', 'CHANGE', 'WAIT_DOTS'], {
        'eng': ['Card Trade', 'Your offer', 'Their offer', 'Your spare cards', 'You have no spare cards.', ' wants to trade cards.', 'Accept', 'Leave', 'Waiting for ', 'Ready', 'Confirm', 'Trade done!', 'Trade cancelled.', " doesn't want to trade.", "You can't hold that many copies.", '< >: amount', 'Confirmed', 'Nothing', '5 different cards at most.', 'Change', 'Waiting...'],
        'fra': ['Échange de cartes', 'Ton offre', 'Son offre', 'Tes cartes en trop', "Tu n'as pas de cartes en trop.", ' veut échanger des cartes.', 'Accepter', 'Quitter', 'On attend ', 'Prêt', 'Confirmer', 'Échange fait !', 'Échange annulé.', ' ne veut pas échanger.', 'Tu ne peux pas avoir autant de copies.', '< > : quantité', 'Confirmé', 'Rien', '5 cartes différentes au plus.', 'Modifier', 'Attente...'],
        'ita': ['Scambio di carte', 'La tua offerta', 'La sua offerta', 'Le tue carte in più', 'Non hai carte in più.', ' vuole scambiare carte.', 'Accetta', 'Esci', 'In attesa di ', 'Pronto', 'Conferma', 'Scambio fatto!', 'Scambio annullato.', ' non vuole scambiare.', 'Non puoi avere tante copie.', '< >: quantità', 'Confermato', 'Niente', 'Al massimo 5 carte diverse.', 'Cambia', 'In attesa...'],
        'ger': ['Kartentausch', 'Dein Angebot', 'Sein Angebot', 'Deine übrigen Karten', 'Du hast keine übrigen Karten.', ' will Karten tauschen.', 'Annehmen', 'Gehen', 'Warte auf ', 'Bereit', 'Bestätigen', 'Tausch erledigt!', 'Tausch abgebrochen.', ' will nicht tauschen.', 'So viele Kopien passen nicht.', '< >: Anzahl', 'Bestätigt', 'Nichts', 'Höchstens 5 verschiedene Karten.', 'Zurück', 'Warte...'],
        'spn': ['Intercambio de cartas', 'Tu oferta', 'Su oferta', 'Tus cartas sueltas', 'No tienes cartas sueltas.', ' quiere intercambiar cartas.', 'Aceptar', 'Salir', 'Esperando a ', 'Listo', 'Confirmar', '¡Intercambio hecho!', 'Intercambio cancelado.', ' no quiere intercambiar.', 'No te caben tantas copias.', '< >: cantidad', 'Confirmado', 'Nada', 'Máximo 5 cartas distintas.', 'Cambiar', 'Esperando...'],
    }),
    ('GUARD', ['TALK', 'TRADE', 'WAIT_OTHER', 'BATTLE', 'PREPARING', 'RAID', 'EXIT', 'DUEL'], {
        'eng': ['Talk', 'Trade cards', 'Waiting for another player...', 'Battle', 'Getting the battle ready...', 'Raid', 'Exit', 'Duel'],
        'fra': ['Parler', 'Échanger des cartes', 'On attend un autre joueur...', 'Combattre', 'On prépare le combat...', 'Raid', 'Quitter', 'Duel'],
        'ita': ['Parla', 'Scambia carte', 'In attesa di un altro giocatore...', 'Combatti', 'Si prepara la lotta...', 'Raid', 'Esci', 'Duello'],
        'ger': ['Reden', 'Karten tauschen', 'Warte auf einen anderen Spieler...', 'Kämpfen', 'Der Kampf wird vorbereitet...', 'Raid', 'Verlassen', 'Duell'],
        'spn': ['Hablar', 'Intercambiar cartas', 'Esperando a otro jugador...', 'Combatir', 'Preparando el combate...', 'Raid', 'Salir', 'Duelo'],
    }),
    ('RAID', ['LEVEL', 'OF_FOUR', 'WAITING', 'DOTS', 'GONE', 'TURN', 'YOUR_TURN', 'FLED'], {
        'eng': ['Lv', ' of 4', 'Waiting for ', '...', ' disconnected.', 'Turn: ', 'Your turn!', ' ran away.'],
        'fra': ['Nv', ' sur 4', 'On attend ', '...', " s'est déconnecté.", 'Au tour de ', "C'est ton tour !", " s'est enfui."],
        'ita': ['Lv', ' su 4', 'In attesa di ', '...', ' ha perso la connessione.', 'Turno di ', 'Tocca a te!', ' è fuggito.'],
        'ger': ['Lv', ' von 4', 'Warte auf ', '...', ' hat die Verbindung verloren.', 'Am Zug: ', 'Du bist dran!', ' ist geflohen.'],
        'spn': ['Nv', ' de 4', 'Esperando a ', '...', ' se ha desconectado.', 'Turno de ', '¡Tu turno!', ' ha huido.'],
    }),
    ('MENU', ['OPTIONS'], {
        'eng': ['OPTIONS'],
        'fra': ['OPTIONS'],
        'ita': ['OPZIONI'],
        'ger': ['OPTIONEN'],
        'spn': ['OPCIONES'],
    }),
    # SHOP the item shop's confirmation (stitshop/trade.c): the disc's Spanish has "Buy" and "Sell" in English there
    ('SHOP', ['BUY', 'SELL'], {
        'eng': ['Buy', 'Sell'],
        'fra': ['Acheter', 'Vendre'],
        'ita': ['Compra', 'Vendi'],
        'ger': ['Kaufen', 'Verkaufen'],
        'spn': ['Comprar', 'Vender'],
    }),
    # FORMS the lab's chart (stgdglab/coop_forms.inc.c): the partner's forms, a form's line and what each needs.
    # TITLE_PRE + the partner's name + TITLE_POST; STAT_1..14 as STFGTREP's Evolution.stat
    ('FORMS', ['WORD', 'TITLE_PRE', 'TITLE_POST', 'LINE', 'LINE_TITLE', 'BACK', 'LV', 'OWNED', 'NEXT', 'READY', 'NONE',
               'STAT_1', 'STAT_2', 'STAT_3', 'STAT_4', 'STAT_5', 'STAT_6', 'STAT_7', 'STAT_8', 'STAT_9', 'STAT_10',
               'STAT_11', 'STAT_12', 'STAT_13', 'STAT_14'], {
        'eng': ['Forms', '', "'s Forms", 'Line', 'Line: ', 'Back', 'Lv', 'Learned', 'Next battle', 'Ready', 'No conditions',
                'Strength', 'Defense', 'Spirit', 'Wisdom', 'Speed', 'Charisma', 'Level', 'Fire Res.', 'Water Res.',
                'Ice Res.', 'Wind Res.', 'Thunder Res.', 'Machine Res.', 'Dark Res.'],
        'fra': ['Formes', 'Formes de ', '', 'Lignée', 'Lignée : ', 'Retour', 'Nv', 'Apprise', 'Prochain combat', 'Prête',
                'Aucune condition',
                'Force', 'Défense', 'Esprit', 'Sagesse', 'Vitesse', 'Charisme', 'Niveau', 'Rés. Feu', 'Rés. Eau',
                'Rés. Glace', 'Rés. Vent', 'Rés. Foudre', 'Rés. Machine', 'Rés. Ténèbres'],
        'ita': ['Forme', 'Forme di ', '', 'Linea', 'Linea: ', 'Indietro', 'Lv', 'Appresa', 'Prossima lotta', 'Pronta',
                'Nessuna condizione',
                'Forza', 'Difesa', 'Spirito', 'Saggezza', 'Velocità', 'Carisma', 'Livello', 'Res. Fuoco', 'Res. Acqua',
                'Res. Ghiaccio', 'Res. Vento', 'Res. Tuono', 'Res. Macchina', 'Res. Oscurità'],
        'ger': ['Formen', '', 's Formen', 'Linie', 'Linie: ', 'Zurück', 'Lv', 'Gelernt', 'Nächster Kampf', 'Bereit',
                'Keine Bedingungen',
                'Stärke', 'Abwehr', 'Geist', 'Weisheit', 'Tempo', 'Charisma', 'Level', 'Feuer-Res.', 'Wasser-Res.',
                'Eis-Res.', 'Wind-Res.', 'Donner-Res.', 'Maschinen-Res.', 'Dunkel-Res.'],
        'spn': ['Formas', 'Formas de ', '', 'Línea', 'Línea: ', 'Volver', 'Nv', 'Aprendida', 'Próximo combate', 'Lista',
                'Sin condiciones',
                'Fuerza', 'Defensa', 'Espíritu', 'Sabiduría', 'Velocidad', 'Carisma', 'Nivel', 'Res. Fuego', 'Res. Agua',
                'Res. Hielo', 'Res. Viento', 'Res. Rayo', 'Res. Máquina', 'Res. Oscuridad'],
    }),
]


def strings():
    """{language: every string in order}, and [(define, index)]"""
    texts = {lang: [""] for lang in game_text.LANGUAGES}
    ids = [("COOP_TEXT_EMPTY", 0)]
    for block, names, by_lang in BLOCKS:
        for lang in game_text.LANGUAGES:
            if len(by_lang[lang]) != len(names):
                sys.exit(f"{block}: {lang} has {len(by_lang[lang])} strings, not {len(names)}")
        ids += [(f"COOP_TEXT_{block}_{n}", len(texts["eng"]) + i) for i, n in enumerate(names)]
        for lang in game_text.LANGUAGES:
            texts[lang] += by_lang[lang]
    return texts, ids


def main():
    texts, ids = strings()
    game_text.write_header("platform/coop_text.h", "tools/coop_text.py", "COOP_TEXT", texts)
    h = ["/* Generated by tools/coop_text.py: do not edit. Each co-op string's index in native_coop_text's file */", ""]
    h += [f"#define {name} {i}" for name, i in ids]
    (game_text.NATIVE / "include/coop_text_ids.h").write_text("\n".join(h + [""]), encoding="utf-8", newline="\n")
    print("include/coop_text_ids.h")


if __name__ == "__main__":
    main()
