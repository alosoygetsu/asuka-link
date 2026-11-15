#!/usr/bin/env python3
"""Exports every quest of the Digilog, as the game shows it, and what lies behind each step, for a local viewer.

    python tools/misiones.py [--out DIR] [--partida game_N.bin]

Writes to native/build/misiones/ (ignored by git: the dialogue is the game's, it never goes to the repo):
  data.js      the story's steps and the side quests, their steps in the 5 languages, each step's condition, map,
               the talks and events that set it (with the character and their sprite), what they give and take,
               the quests they lead to and the warnings found
  index.html   the viewer (a copy of tools/misiones_viewer.html): open it with a double click

It reuses what already reads the game: the steps of tools/journal_steps.py, the port's quest list (SIDE_QUESTS of
ststatus/item_list.c, patched), the texts of tools/packs_text/*.toml, the talks of the stages' JSON (as
tools/quest_talks.py), the flag sites of dw3.code_sites (as dw3.py marca) and the drops of dw3.enemy_table.
The characters' sprites come from the sprites viewer (tools/sprites.py -> build/sprites/), if it was generated.
With --partida, a GAME dump (probar.ps1 campo -Press "500:f2" -> testN/game_*.bin) marks the steps it has done.
"""
import argparse
import json
import re
import shutil
import struct
import sys
import tomllib
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
import dw3  # noqa: E402
import journal_steps as J  # noqa: E402
import quest_talks as Q  # noqa: E402

NATIVE = TOOLS.parent
OUT = NATIVE / "build" / "misiones"
VIEWER = TOOLS / "misiones_viewer.html"
ITEM_LIST = NATIVE / "build" / "patched" / "src" / "ststatus" / "item_list.c"
MAP_SCREEN = NATIVE / "build" / "patched" / "src" / "ststatus" / "map_screen.c"

# (code of tools/packs_text and journal_text, label, letter of the item names' file, index of a stage's text file)
LANGS = [("spn", "ES", "S", 4), ("eng", "EN", "E", 0), ("fra", "FR", "F", 1), ("ita", "IT", "I", 2), ("ger", "DE", "D", 3)]
TEXT_STEP = 18  # packs_text: 18 + 2p each story step's title, then its text
TEXT_SIDE_QUEST = 113  # packs_text: the port's side quests' texts
PORT_SIDE_QUESTS = 29

# the side quests' kinds, by their key
KINDS = {
    "ddna": {item for item, *_ in J.DDNA},
    "armas": {0x069, 0x077, 0x083, 0x090, 0x09C},
    "cartas": {0x012, 0x014, 0x192, 0x200, 0x201},
    "subastas": {0x202},
    "jefes": {0x066, 0x074, 0x099, 0x08D, 0x0DD, 0x088, 0x0A3, 0x030, 0x033, 0x141, 0x18C, 0x02A, 0x0F2, 0x0F5,
              0x100, 0x10E, 0x142, 0x0CB, 0x0F0},
}


def kind_of(key):
    return next((k for k, keys in KINDS.items() if key in keys), "otras")


def norm(c):
    """a condition's code as journal_steps writes it: an item without its kind (0x8000 | id)"""
    return 0x8000 | c & 0x1FF if 0x8000 <= c < 0x9000 else c


def hexs(n, w=3):
    return f"0x{n:0{w}X}"


# ---------------------------------------------------------------- the game's texts

def packs_texts():
    return {lang: tomllib.loads((TOOLS / "packs_text" / f"{lang}.toml").read_text(encoding="utf-8"))["strings"]
            for lang, *_ in LANGS}


def item_names():
    """[[ES, EN, FR, IT, DE] per item id], with our names where the game's has a typo (journal_steps.ITEM_NAMES)"""
    tables = [dw3.names_of("ITMNAM", letter) for _, _, letter, _ in LANGS]
    out = [[t[i] if i < len(t) else "" for t in tables] for i in range(max(len(t) for t in tables))]
    for i, es, en in J.ITEM_NAMES:
        out[i][0], out[i][1] = es, en
    return out


class Translator:
    """a Digilog text of journal_steps (Spanish, English) in the five languages, as the game gets them: French,
    Italian and German from tools/journal_text, English where they lack it (and then it says so)"""

    def __init__(self):
        self.tables = {lang: J.translations(lang) for lang in J.TRANSLATED}

    def __call__(self, es, en):
        texts, missing = [es, en], []
        for lang, label, _, _ in LANGS[2:]:
            tr = self.tables[lang].get(en.replace("\n", " "))
            if tr is None:
                missing.append(label)
            texts.append(tr if tr is not None else en)
        return [self.wrap(t) for t in texts], missing

    @staticmethod
    def wrap(s):
        try:
            return J.wrap(s)
        except SystemExit:
            return s


# ---------------------------------------------------------------- maps

def maps():
    """{mode: {stage, name, zone}} of every stage's modes"""
    places = dw3.stage_places()
    out = {}
    for st, d in dw3.stages_json().items():
        for m in d.get("euModes") or d.get("modes") or []:
            name, zone = places.get(st, ("?", "?"))
            out.setdefault(m[0], {"stage": st, "name": name, "zone": zone})
    return out


def stage_modes(stage):
    d = dw3.stages_json().get(stage) or {}
    return [m[0] for m in d.get("euModes") or d.get("modes") or []]


def where_mode(where):
    return J.mode_of(where) if where else 0


# ---------------------------------------------------------------- talks, events and drops behind a code

class Talks:
    """every talk that does something, indexed by the codes of its actions and conditions (as quest_talks)"""

    def __init__(self):
        self.list = []
        self.by_action, self.by_condition = {}, {}
        seen = {}
        for st, d in dw3.stages_json().items():
            modes = [m[0] for m in d.get("euModes") or d.get("modes") or []]
            for ai, a in enumerate(d.get("actors", [])):
                for ti, t in enumerate(a.get("talks", [])):
                    acts = [tuple(c) for c in t.get("actions", []) if c[0] != 0xFFFF]
                    conds = [tuple(c) for c in t.get("conditions", []) if c[0] != 0xFFFF]
                    key = (st, a["id"], t["entry"], tuple(conds), tuple(acts))
                    if key in seen:
                        self.list[seen[key]]["where"].append(f"actor{ai}Talk{ti}")
                        continue
                    seen[key] = len(self.list)
                    actor_conds = [tuple(c) for c in a.get("conditions", []) if c[0] != 0xFFFF]
                    self.list.append(dict(stage=st, modes=modes, char=a["id"], entry=t["entry"], conds=conds,
                                          acts=acts, actor=actor_conds, where=[f"actor{ai}Talk{ti}"],
                                          text=d.get("textFile")))
                    for c, v in acts:
                        self.by_action.setdefault(norm(c), []).append((seen[key], v))
                    for c, v in conds:
                        self.by_condition.setdefault(norm(c), []).append((seen[key], v))
        self._said = {}

    def said(self, k):
        """what talk k says, in the five languages"""
        if k not in self._said:
            t = self.list[k]
            out = []
            for *_, idx in LANGS:
                try:
                    b = dw3.game_file(t["text"] + idx)
                    o = struct.unpack_from("<I", b, 4 * (t["entry"] + 1))[0]
                    out.append(dw3.decode(b[o:o + 600]))
                except Exception:
                    out.append("")
            self._said[k] = out
        return self._said[k]

    def row(self, k, v=None):
        t = self.list[k]
        words = lambda cs: [x for x in (Q.code(c, val) for c, val in cs) if x]
        return {"stage": t["stage"], "mode": t["modes"][0] if t["modes"] else 0, "char": t["char"],
                "where": ",".join(t["where"]), "talk": t["entry"], "say": self.said(k),
                "if": words(t["conds"]), "do": words(t["acts"]), "shown": words(t["actor"]), "val": v}


EVENT_LANGS = [6, 2, 3, 4, 5]  # a stage's event texts: EVENT_TEXT_FILE + this, in LANGS' order
_events = {}


def event_info(stage, n):
    """who speaks in event n of a stage and its first and last lines, in the five languages (story_dialogue.py)"""
    if (stage, n) in _events:
        return _events[(stage, n)]
    import story_dialogue as sd
    out = {"event": n, "chars": [], "lines": []}
    r = sd.find(n, stage)
    if r:
        _, base, part, script = r
        said = [(int(m.group(1), 0), int(m.group(2), 0)) for m in
                re.finditer(r"0x200,\s*\w+,\s*(0x[0-9A-Fa-f]+|\d+),\s*(0x[0-9A-Fa-f]+|\d+)", script)]
        out["chars"] = list(dict.fromkeys(who for _, who in said if who > 2))
        out["count"] = len(said)
        if base is not None and part is not None:
            idx = [i for i, _ in said]
            idx = idx if len(idx) <= 4 else idx[:2] + idx[-2:]
            tables = []
            for k in EVENT_LANGS:
                try:
                    tables.append(sd.archive_entry((sd.FILES / ("%04d.bin" % (base + k))).read_bytes(), part))
                except OSError:
                    tables.append(None)
            for i in idx:
                line = []
                for t in tables:
                    try:
                        line.append(dw3.decode(Q.entry(t, i, 600)) if t else "")
                    except Exception:
                        line.append("")
                out["lines"].append(line)
    _events[(stage, n)] = out
    return out


def site_row(s):
    stage = s["file"].stem if s["file"].stem.startswith("wstag") else ""
    modes = stage_modes(stage) if stage else []
    row = {"where": dw3.rel(s["file"], s["line"]), "ctx": s["ctx"], "stage": stage, "mode": modes[0] if modes else 0,
           "kind": s["kind"], "text": s["text"][:110]}
    m = re.match(r"endEvent(\d+)$", s["ctx"])
    if m and stage:
        row.update(event_info(stage, int(m.group(1))))
    return row


class Evidence:
    """who sets, clears, tests or gives a code: talks, the code's sites (events, scripts), the Cargo Tower's
    choices and the enemies' drops"""

    def __init__(self):
        self.talks = Talks()
        self.sites = {}
        for s in dw3.code_sites():
            if re.match(r"actor\d+Talk\d+", s["ctx"]):  # the stages' JSON has these, with their character
                continue
            self.sites.setdefault(s["code"], []).append(s)
        self.choices = dw3.story_choices()
        self.enemies = dw3.enemy_table()
        self.places = dw3.enemy_places()

    def of(self, code):
        """{set, clear, test, sites, drops, choices} of a code (flag, item or card)"""
        t = self.talks
        acts = t.by_action.get(code, [])
        out = {
            "set": [t.row(k, v) for k, v in acts if v],
            "clear": [t.row(k, v) for k, v in acts if not v],
            "test": [t.row(k, v) for k, v in t.by_condition.get(code, [])][:12],
            "sites": [site_row(s) for s in self.sites.get(code, []) if s["kind"] in ("set", "clear")],
            "choices": [],
            "drops": [],
        }
        for r in self.choices:
            if r["flag"] == code and r.get("answer"):
                out["choices"].append({"where": r["answer"], "text": f"subasta del paso {r['progress']:#x}: "
                                       f"las dos respuestas ponen la marca; pujar, también {', '.join(r['actions'])}",
                                       "mode": 0x210})
            if any(norm(c) == code for c in r.get("codes", ())):
                out["choices"].append({"where": r["answer"], "text": f"subasta del paso {r['progress']:#x}: "
                                       f"al pujar ({', '.join(r['actions'])})", "mode": 0x210})
        if 0x8000 <= code < 0x9000:
            item = code & 0x1FF
            for f, e in sorted(self.enemies.items(), key=lambda kv: -kv[1]["chance"]):
                if e["item"] == item:
                    where = sorted({st for st, *_ in self.places.get(f, [])})
                    out["drops"].append({"enemy": dw3.enemy_name(f), "chance": round((e["chance"] + 1) * 100 / 1024, 1),
                                         "maps": [(st, stage_modes(st)[0] if stage_modes(st) else 0) for st in where[:6]]})
        return out


# ---------------------------------------------------------------- the quests

def port_side_quests():
    """the port's SIDE_QUESTS rows (item_list.c, patched): (item, mode, done code, last step, item needed)"""
    t = ITEM_LIST.read_text(encoding="utf-8")
    body = t[t.index("static const SideQuest SIDE_QUESTS[] = {"):]
    body = body[:body.index("JOURNAL_NEW_SIDE_QUESTS")]
    rows = []
    for m in re.finditer(r"\{ (0x[0-9A-Fa-f]+), (0x[0-9A-Fa-f]+), (FLAG\((0x[0-9A-Fa-f]+), (0x[0-9A-Fa-f]+)\)|"
                         r"JOURNAL_PARTNER_DDNA\((\d+), (0x[0-9A-Fa-f]+)\))(?:, (0x[0-9A-Fa-f]+|\d+))?(?:, (0x[0-9A-Fa-f]+))? \}",
                         body):
        item, mode = int(m.group(1), 16), int(m.group(2), 16)
        done = (int(m.group(4), 16) << 8 | int(m.group(5), 16)) if m.group(4) else J.PARTNER_DDNA(int(m.group(6)), int(m.group(7), 16))
        rows.append((item, mode, done, int(m.group(8), 0) if m.group(8) else 0, int(m.group(9), 16) if m.group(9) else 0))
    if len(rows) != PORT_SIDE_QUESTS:
        sys.exit(f"{ITEM_LIST}: {len(rows)} filas en SIDE_QUESTS, se esperaban {PORT_SIDE_QUESTS}")
    return rows


def objective_modes():
    t = MAP_SCREEN.read_text(encoding="utf-8")
    body = t[t.index("objectiveModes[] = {"):]
    return [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", body[:body.index("};")])]


def code_kind(c):
    if c == J.ALL:
        return "all"
    if c >= 0xF000:
        return "after"
    if c >= 0xD000:
        return "ddna"
    if 0x9200 <= c < 0x9400:
        return "card"
    if 0x8000 <= c < 0x9000:
        return "item"
    if 0x7200 <= c < 0x7400:
        return "party"
    if 0x7000 <= c < 0x7200:
        return "special"
    return "flag"


def build(dump=None):
    ev = Evidence()
    tr = Translator()
    packs = packs_texts()
    items = item_names()
    mp = maps()
    objectives = objective_modes()
    quests = []

    def step(sid, code, where, es, en):
        texts, missing = tr(es, en)
        mode = where_mode(where)
        kind = code_kind(code)
        s = {"id": sid, "code": code, "kind": kind, "cond": dw3.journal_code(code), "mode": mode, "text": texts,
             "untranslated": missing, "warn": [], "info": []}
        if kind in ("flag", "item", "card"):
            s["ev"] = ev.of(code)
        elif kind == "ddna":
            s["ev"] = ev.of(0x8000 | code & 0x1FF)
        if kind == "after":
            s["link"] = f"h{code & 0xFFF:02X}"
            s["linkText"] = f"historia {code & 0xFFF:#x}"
        if kind in ("item", "ddna"):
            s["item"] = code & 0x1FF
        if kind == "card":
            s["card"] = code & 0x1FF
        return s

    # the story: one entry per step of GAME.progress, with its group's steps
    groups = {p: steps for steps_of, steps in J.STORY for p in steps_of}
    group_of = {p: steps_of for steps_of, _ in J.STORY for p in steps_of}
    for p in range(len(objectives)):
        steps = [step(f"h{p:02X}.{n}", c, w, es, en) for n, (c, w, es, en) in enumerate(groups.get(p, []))]
        quests.append({"id": f"h{p:02X}", "type": "historia", "kind": "historia", "progress": p,
                       "title": [packs[lang][TEXT_STEP + 2 * p] for lang, *_ in LANGS],
                       "text": [packs[lang][TEXT_STEP + 2 * p + 1] for lang, *_ in LANGS], "mode": objectives[p],
                       "group": [f"h{x:02X}" for x in group_of.get(p, [])], "steps": steps, "warn": [], "info": []})

    # the side quests: the port's first (their texts in packs_text), then ours (journal_steps)
    titled = {q[0]: q for q in J.TITLED_QUESTS}
    side = [(item, mode, done, last, req, [packs[lang][TEXT_SIDE_QUEST + i] for lang, *_ in LANGS], [], "port")
            for i, (item, mode, done, last, req) in enumerate(port_side_quests())]
    for key, where, done, es, en in J.NEW_QUESTS:
        texts, missing = tr(es, en)
        req = titled[key][2] if key in titled else 0
        side.append((key, J.mode_of(where), done, J.QUEST_LAST.get(key, 0), req, texts, missing, "asuka"))
    for key, mode, done, last, req, texts, missing, origin in side:
        if key in titled:
            title, _ = tr(titled[key][3], titled[key][4])
        else:
            title = items[key]
        steps = [step(f"q{key:03X}.{n}", c, w, es, en) for n, (c, w, es, en) in enumerate(J.QUEST_STEPS.get(key, []))]
        q = {"id": f"q{key:03X}", "type": "mision", "kind": kind_of(key), "key": key, "origin": origin, "title": title,
             "text": texts, "untranslated": missing, "mode": mode, "done": done, "doneText": dw3.journal_code(done),
             "last": last, "req": req, "reward": key if key < 0x200 else 0, "steps": steps, "warn": [], "info": []}
        if done not in (J.ALL,) and code_kind(done) == "flag":
            q["doneEv"] = ev.of(done)
        quests.append(q)

    check(quests, mp)
    links(quests)
    data = {"langs": [label for _, label, _, _ in LANGS], "maps": {str(m): v for m, v in mp.items()},
            "items": items, "chars": {str(k): dw3.character_name(k) for k in characters(quests)},
            "quests": quests}
    if dump:
        data["state"] = save_state(dump, quests)
    return data


def characters(quests):
    out = set()
    for q in quests:
        for s in q["steps"]:
            for part in ("set", "clear", "test"):
                out |= {t["char"] for t in s.get("ev", {}).get(part, [])}
            for x in s.get("ev", {}).get("sites", []):
                out |= set(x.get("chars", []))
    return sorted(out)


# ---------------------------------------------------------------- warnings and links

def check(quests, mp):
    """what may be wrong in a step (warn) or is worth knowing (info)"""
    for q in quests:
        for s in q["steps"]:
            e = s.get("ev")
            if s["untranslated"]:
                s["info"].append(f"Sin traducir (sale en inglés): {', '.join(s['untranslated'])}")
            if not e:
                continue
            setters = e["set"] + [x for x in e["sites"] if x["kind"] == "set"] + e["choices"]
            if s["kind"] == "flag":
                if not setters:
                    s["warn"].append("Nadie pone esta marca: ni una charla, ni un evento, ni una subasta.")
                clears = e["clear"] + [x for x in e["sites"] if x["kind"] == "clear"]
                if clears:
                    s["warn"].append("La marca se borra luego (" + ", ".join(
                        sorted({c.get("stage") or c.get("where", "?") for c in clears})) +
                        "): al borrarse, el paso deja de contar como hecho.")
            elif s["kind"] in ("item", "ddna", "card") and not setters and not e["drops"]:
                s["info"].append("Ninguna charla, evento ni enemigo lo da: ¿una tienda o un cofre?")
            modes = {x["mode"] for x in setters if x.get("mode")}
            if s["kind"] == "flag" and s["mode"] and modes and s["mode"] not in modes and all(x.get("stage") or x.get("mode") for x in setters):
                names = ", ".join(f"{hexs(m)} {mp.get(m, {}).get('name', '?').split(',')[0]}" for m in sorted(modes))
                s["warn"].append(f"El paso apunta a {hexs(s['mode'])} {mp.get(s['mode'], {}).get('name', '?').split(',')[0]}, "
                                 f"pero se cumple en {names}.")
            if s["mode"] and s["mode"] not in mp and s["mode"] != 0x210:
                s["warn"].append(f"El mapa {hexs(s['mode'])} no es de ningún escenario.")
        if q["type"] == "mision":
            if q.get("doneEv") is not None:
                e = q["doneEv"]
                if not (e["set"] or [x for x in e["sites"] if x["kind"] == "set"] or e["choices"]):
                    q["warn"].append(f"Nadie pone la marca de hecha ({q['doneText']}).")
            codes = [s["code"] for s in q["steps"]]
            if q["steps"] and q["done"] not in (J.ALL,) and q["done"] not in codes:
                q["info"].append(f"Se da por hecha con {q['doneText']}, que no es ninguno de sus pasos.")
            if q["untranslated"]:
                q["info"].append(f"Su texto sin traducir (sale en inglés): {', '.join(q['untranslated'])}")
        if q["mode"] and q["mode"] not in mp and q["mode"] != 0x210:
            q["warn"].append(f"Su mapa {hexs(q['mode'])} no es de ningún escenario.")


def links(quests):
    """each quest's links: the quests it needs (an item that's another's reward, a past story step) and those that
    need it"""
    by_key = {q["key"]: q for q in quests if q["type"] == "mision"}
    by_id = {q["id"]: q for q in quests}
    for q in quests:
        q.setdefault("needs", [])
        q.setdefault("neededBy", [])
    for q in quests:
        want = []
        if q.get("req") and q["req"] in by_key:
            want.append((by_key[q["req"]]["id"], "la pide antes"))
        for s in q["steps"]:
            if s.get("item") in by_key and by_key[s["item"]] is not q:
                s["quest"] = by_key[s["item"]]["id"]
                want.append((s["quest"], "un paso la pide"))
            if s.get("link") in by_id and s["link"] != q["id"] and s["link"] not in q.get("group", []):
                want.append((s["link"], "tras ese paso de historia"))
        for qid, why in dict.fromkeys(want):
            q["needs"].append([qid, why])
            by_id[qid]["neededBy"].append([q["id"], why])


# ---------------------------------------------------------------- a save (GAME dump)

def save_state(path, quests):
    """the steps a GAME dump has done: {file, name, progress, steps: {id: 1, 0 or null (unknown)}, quests}"""
    g = Path(path).read_bytes()
    s32 = lambda o: struct.unpack_from("<i", g, o)[0]
    progress = s32(dw3.GS_PROGRESS)
    groups = dw3.eu_flag_groups()
    names = [dw3.dname(p) for p in range(8)]
    unlocked = {names[p]: s32(dw3.GS_PARTNERS + p * dw3.PARTNER_SIZE + 4) != 0 for p in range(8)}
    level = {names[p]: struct.unpack_from("<h", g, dw3.GS_PARTNERS + p * dw3.PARTNER_SIZE + 0xC + 0x1C)[0]
             for p in range(8)}

    def flag(code):
        grp, n = code >> 8 & 0xFE, code & 0x1FF
        if grp not in groups:
            return None  # FLAGS_00: not in GAME
        return 1 if g[groups[grp] + (n >> 3)] & 1 << (n & 7) else 0

    def item(i):
        return 1 if g[0x7C + i] or g[0x20F + i] else 0

    def held(c):
        k = code_kind(c)
        if k == "after":
            return 1 if progress > c & 0xFFF else 0
        if k == "item":
            return item(c & 0x1FF)
        if k == "card":
            return 1 if g[0x3A2 + (c & 0x1FF)] else 0
        if k == "ddna":
            who = J.PARTNERS_DDNA[c >> 9 & 7]
            return 1 if item(c & 0x1FF) and unlocked.get(who) else 0
        if k == "flag":
            return flag(c)
        if k == "special" and (c & 0x1FF) in Q.SPECIALS:
            kind, arg = Q.SPECIALS[c & 0x1FF]
            if kind == 0x14 and arg < 8:  # a partner at level 45
                return 1 if level.get(Q.PARTNERS[arg], 0) >= 45 else 0
            if kind == 0x11 and arg < 8:
                return 1 if unlocked.get(Q.PARTNERS[arg]) else 0
        return None  # the party's stats need computeStats: unknown here

    steps, done = {}, {}
    for q in quests:
        for s in q["steps"]:
            steps[s["id"]] = held(s["code"])
        if q["type"] == "mision":
            done[q["id"]] = (all(steps[s["id"]] for s in q["steps"]) if q["done"] == J.ALL and q["steps"]
                             else held(q["done"]))
        else:
            done[q["id"]] = 1 if progress > q["progress"] else 0
    return {"file": str(path), "name": dw3.decode(g[0x54:0x6C]), "money": s32(0x6C), "progress": progress,
            "steps": steps, "done": done}


def main():
    ap = argparse.ArgumentParser(description="exporta las misiones del Digilog para el visor")
    ap.add_argument("--out", type=Path, default=OUT, help=f"carpeta de salida (por defecto {OUT})")
    ap.add_argument("--partida", nargs="?", const="ultima", help="un volcado de GAME (game_N.bin); sin archivo, el más reciente")
    a = ap.parse_args()
    dump = None
    if a.partida:
        dump = dw3.latest_dump() if a.partida == "ultima" else Path(a.partida)
        if not dump or not dump.exists():
            sys.exit("no hay volcado (probar.ps1 campo -Press \"500:f2\" lo deja en test5/game_<vsync>.bin)")
    data = build(dump)
    a.out.mkdir(parents=True, exist_ok=True)
    (a.out / "data.js").write_text("window.MISIONES = " + json.dumps(data, ensure_ascii=False, separators=(",", ":"))
                                   + ";\n", encoding="utf-8")
    shutil.copyfile(VIEWER, a.out / "index.html")
    qs = data["quests"]
    nsteps = sum(len(q["steps"]) for q in qs)
    nwarn = sum(len(q["warn"]) + sum(len(s["warn"]) for s in q["steps"]) for q in qs)
    print(f"{len(qs)} entradas ({sum(q['type'] == 'historia' for q in qs)} de historia), {nsteps} pasos, {nwarn} avisos"
          + (f", partida {dump.name} (historia {data['state']['progress']:#x})" if dump else "") + f" -> {a.out / 'index.html'}")


if __name__ == "__main__":
    main()
