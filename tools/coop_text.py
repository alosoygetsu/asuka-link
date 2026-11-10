"""The co-op build's own texts in the game's text encoding, into platform/coop_text.h: one text file per language
(a count, each string's offset, the strings), as the game's.

The encoding is learned from the port's own texts (tools/packs_text/<language>.toml and the platform/packs_text.h
that ddw3's mklang-file made of them): a letter is one byte, anything else two. So this needs no outside tool, only
characters those texts have. Run from anywhere: python native/tools/coop_text.py

Index: 0 "", 1 the team choice's title, 2 its help, 3 the confirmation's "Your team", 4 the welcome's sentence
about the partners (in place of the one about the three packs), 5-7 the picks' marks "1" "2" "3", 8 the team's
separator ", ", 9 the team line before any pick,
10 the team page's Accept, 11 the look page's title, 12 its help, 13-15 the models' names, 16-36 the card trade's.
"""
import re
import sys
import tomllib
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
LANGUAGES = ["eng", "fra", "ita", "ger", "spn"]

TEXTS = {
    "eng": ["", "Choose your Partners", "Pick 3 partners; pick one again\nto drop it. Then Accept.", "Your Team",
            "Please choose three partner\nDigimon, any three you like.", "1", "2", "3", ", ", "No partners yet", "Accept", "Choose your Look", "Pick who you are and a colour.", "Junior", "Boy", "Girl"],
    "fra": ["", "Choisis tes partenaires", "Choisis 3 partenaires; le choisir\nà nouveau le retire. Puis Valider.",
            "Ton équipe", "Choisis trois Digimon\npartenaires, ceux que tu veux.", "1", "2", "3", ", ",
            "Aucun partenaire", "Valider", "Choisis ton look", "Choisis qui tu es et une couleur.", "Junior", "Garçon", "Fille"],
    "ita": ["", "Scegli i tuoi partner", "Scegli 3 partner; sceglierlo di\nnuovo lo toglie. Poi Conferma.",
            "La tua squadra", "Scegli tre Digimon partner,\nquelli che preferisci.", "1", "2", "3", ", ",
            "Nessun partner", "Conferma", "Scegli il tuo aspetto", "Scegli chi sei e un colore.", "Junior", "Ragazzo", "Ragazza"],
    "ger": ["", "Wähle deine Partner", "Wähle 3 Partner; erneut wählen\nentfernt ihn. Dann Weiter.", "Dein Team",
            "Wähle drei Partner-Digimon,\nganz nach deinem Geschmack.", "1", "2", "3", ", ", "Noch keine Partner", "Weiter", "Wähle dein Aussehen", "Wähle, wer du bist, und eine Farbe.", "Junior", "Junge", "Mädchen"],
    "spn": ["", "Elige tus compañeros", "Elige 3 compañeros; elegir otra\nvez lo quita. Luego, Aceptar.",
            "Tu equipo", "Elige a tres Digimon\ncompañeros, los que quieras.", "1", "2", "3", ", ",
            "Sin compañeros aún", "Aceptar", "Elige tu personaje", "Elige quién eres y un color.", "Junior", "Chico", "Chica"],
}


def packs_file(h, lang):
    body = re.search(r"PACKS_TEXT_%s\[\] = \{(.*?)\};" % lang.upper(), h, re.S).group(1)
    return b"".join(int(x, 16).to_bytes(4, "little") for x in re.findall(r"0x([0-9A-F]{8})", body))


def learn():
    """character -> its bytes, from the port's texts and their encoding"""
    h = (NATIVE / "platform/packs_text.h").read_text(encoding="utf-8")
    table = {}
    for lang in LANGUAGES:
        b = packs_file(h, lang)
        n = int.from_bytes(b[:4], "little")
        offs = [int.from_bytes(b[4 + 4 * i:8 + 4 * i], "little") for i in range(n)]
        strings = tomllib.loads((NATIVE / f"tools/packs_text/{lang}.toml").read_text(encoding="utf-8"))["strings"]
        for s, o in zip(strings, offs):
            if "[" in s:
                continue  # a code's length isn't known
            p = o
            for c in s:
                code = b[p:p + (2 if b[p] in (1, 2) else 1)]
                if table.setdefault(c, code) != code:
                    sys.exit(f"{c!r} has two encodings")
                p += len(code)
            if b[p] != 0:
                sys.exit(f"{lang}: {s!r} isn't what its bytes say")
    # the digits run in order from 0: the ones the texts lack
    zero = table["0"]
    for d in range(10):
        want = zero[:-1] + bytes([zero[-1] + d])
        if table.setdefault(str(d), want) != want:
            sys.exit("the digits aren't in order")
    return table


# 16-42: the card trade's (37-39 the Guardromon's choice and the wait, 40-41 the battle's, 42 the raid's, 43 the exit, 44 the raid list's level, 45 the room's " of 4", 46-47 the raid's wait and a player gone) (native_coop.h COOP_TEXT_TRADE_*)
TRADE_TEXTS = {
    "eng": ["Card Trade", "Your offer", "Their offer", "Your spare cards", "You have no spare cards.",
            " wants to trade cards.", "Accept", "Leave", "Waiting for ", "Ready", "Confirm", "Trade done!",
            "Trade cancelled.", " doesn't want to trade.", "You can't hold that many copies.", "< >: amount",
            "Confirmed", "Nothing", "5 different cards at most.", "Change", "Waiting...", "Talk", "Trade cards", "Waiting for another player...", "Battle", "Getting the battle ready...", "Raid", "Exit", "Lv", " of 4", "...", " disconnected.", "Duel"],
    "fra": ["Échange de cartes", "Ton offre", "Son offre", "Tes cartes en trop", "Tu n'as pas de cartes en trop.",
            " veut échanger des cartes.", "Accepter", "Quitter", "On attend ", "Prêt", "Confirmer", "Échange fait !",
            "Échange annulé.", " ne veut pas échanger.", "Tu ne peux pas avoir autant de copies.", "< > : quantité",
            "Confirmé", "Rien", "5 cartes différentes au plus.", "Modifier", "Attente...", "Parler", "Échanger des cartes", "On attend un autre joueur...", "Combattre", "On prépare le combat...", "Raid", "Quitter", "Nv", " sur 4", "...", " s'est déconnecté.", "Duel"],
    "ita": ["Scambio di carte", "La tua offerta", "La sua offerta", "Le tue carte in più", "Non hai carte in più.",
            " vuole scambiare carte.", "Accetta", "Esci", "In attesa di ", "Pronto", "Conferma", "Scambio fatto!",
            "Scambio annullato.", " non vuole scambiare.", "Non puoi avere tante copie.", "< >: quantità",
            "Confermato", "Niente", "Al massimo 5 carte diverse.", "Cambia", "In attesa...", "Parla", "Scambia carte", "In attesa di un altro giocatore...", "Combatti", "Si prepara la lotta...", "Raid", "Esci", "Lv", " su 4", "...", " ha perso la connessione.", "Duello"],
    "ger": ["Kartentausch", "Dein Angebot", "Sein Angebot", "Deine übrigen Karten", "Du hast keine übrigen Karten.",
            " will Karten tauschen.", "Annehmen", "Gehen", "Warte auf ", "Bereit", "Bestätigen", "Tausch erledigt!",
            "Tausch abgebrochen.", " will nicht tauschen.", "So viele Kopien passen nicht.", "< >: Anzahl",
            "Bestätigt", "Nichts", "Höchstens 5 verschiedene Karten.", "Zurück", "Warte...", "Reden", "Karten tauschen", "Warte auf einen anderen Spieler...", "Kämpfen", "Der Kampf wird vorbereitet...", "Raid", "Verlassen", "Lv", " von 4", "...", " hat die Verbindung verloren.", "Duell"],
    "spn": ["Intercambio de cartas", "Tu oferta", "Su oferta", "Tus cartas sueltas", "No tienes cartas sueltas.",
            " quiere intercambiar cartas.", "Aceptar", "Salir", "Esperando a ", "Listo", "Confirmar",
            "¡Intercambio hecho!", "Intercambio cancelado.", " no quiere intercambiar.", "No te caben tantas copias.",
            "< >: cantidad", "Confirmado", "Nada", "Máximo 5 cartas distintas.", "Cambiar", "Esperando...", "Hablar", "Intercambiar cartas", "Esperando a otro jugador...", "Combatir", "Preparando el combate...", "Raid", "Salir", "Nv", " de 4", "...", " se ha desconectado.", "Duelo"],
}
for _lang in TEXTS:
    TEXTS[_lang] = TEXTS[_lang] + TRADE_TEXTS[_lang]


def encode(table, s):
    try:
        return b"".join(table[c] for c in s) + b"\0"
    except KeyError as e:
        sys.exit(f"{s!r}: {e.args[0]!r} isn't in the port's texts, so its encoding isn't known")


def text_file(table, strings):
    head = 4 + 4 * len(strings)
    out, offs = b"", []
    for s in strings:
        offs.append(head + len(out))
        out += encode(table, s)
        out += bytes(-len(out) % 4)
    return len(strings).to_bytes(4, "little") + b"".join(o.to_bytes(4, "little") for o in offs) + out


def main():
    table = learn()
    lines = ["/* Generated by tools/coop_text.py: do not edit */", ""]
    for lang in TEXTS:
        b = text_file(table, TEXTS[lang])
        words = [int.from_bytes(b[i:i + 4], "little") for i in range(0, len(b), 4)]
        lines.append(f"static const uint32_t COOP_TEXT_{lang.upper()}[] = {{")
        lines += ["    " + ", ".join(f"0x{w:08X}" for w in words[i:i + 6]) + "," for i in range(0, len(words), 6)]
        lines += ["};", ""]
    (NATIVE / "platform/coop_text.h").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print("platform/coop_text.h")


if __name__ == "__main__":
    main()
