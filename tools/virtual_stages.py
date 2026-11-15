"""Stages the extracted game (native/build/data/stages) has no JSON for, written the same way from the decomp's C, so
that the tools read them as any other: the field's own stage, mode 0x210 (Asuka City's Cargo Tower: its actors, talks
and exits are FIELDSTG's, fieldstg/data/fieldstg.c and field_stage.c; its events' scripts, wstag260.c).

    import virtual_stages; virtual_stages.paths()  -> [Path of each JSON]   (written to native/build/dw3_virtual/)
"""
import json
import re
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
DECOMP = NATIVE.parent / "dw3_decomp"
OUT = NATIVE / "build" / "dw3_virtual"
VERSION = 1  # raise it when what's written changes

NUM = r"(0x[0-9A-Fa-f]+|-?\d+)"


def _array(src, name):
    """the numbers of `u16 name[] = {...};`"""
    m = re.search(r"\b%s\[\] = \{(.*?)\};" % name, src, re.S)
    return [int(x, 0) for x in re.findall(NUM, re.sub(r"/\*.*?\*/", "", m.group(1), flags=re.S))] if m else []


def _codes(arrays, ref):
    """the (code, value) pairs at &ARRAY[k], up to code 0xFFFF; [] for NULL"""
    m = re.match(r"&(\w+)\[(\d+)\]", ref.strip())
    if not m:
        return []
    a, k, out = arrays.get(m.group(1), []), int(m.group(2)), []
    while k + 1 < len(a) and a[k] != 0xFFFF:
        out.append([a[k], a[k + 1]])
        k += 2
    return out


def field_stage():
    src = (DECOMP / "src" / "fieldstg" / "data" / "fieldstg.c").read_text(encoding="utf-8", errors="replace")
    arrays = {n: _array(src, n) for n in ("FIELDSTG_talkFlags", "FIELDSTG_actorFlags")}
    body = src[src.index("FieldTalk FIELDSTG_talks[] = {"):]
    body = body[:body.index("};")]
    talks = [(c, a, int(e, 0)) for c, a, e in re.findall(r"\{(NULL|&\w+\[\d+\]), (NULL|&\w+\[\d+\]), " + NUM + r"\}", body)]
    body = src[src.index("FieldActorEntry FIELDSTG_actors[] = {"):]
    body = body[:body.index("};")]
    actors = []
    for c, t, aid, image, x, y, d in re.findall(r"\{(NULL|&\w+\[\d+\]), (NULL|&FIELDSTG_talks\[\d+\]), %s, %s, %s, %s, %s\}"
                                                % ((NUM,) * 5), body):
        lst = []
        m = re.match(r"&FIELDSTG_talks\[(\d+)\]", t)
        k = int(m.group(1)) if m else None
        while k is not None and k < len(talks):  # its talks: up to the first without conditions
            tc, ta, entry = talks[k]
            lst.append({"entry": entry, "conditions": _codes(arrays, tc), "actions": _codes(arrays, ta)})
            if tc == "NULL":
                break
            k += 1
        actors.append({"id": int(aid, 0), "image": int(image, 0), "x": int(x, 0), "y": int(y, 0), "dir": int(d, 0),
                       "conditions": _codes(arrays, c), "talks": lst})
    body = src[src.index("StageSlot FIELDSTG_slots[] = {"):]
    body = body[:body.index("};")]
    slots = []
    for m in re.finditer(r"\{\{\{%s, %s\}, \{%s, %s\}\}, %s, %s, %s, %s, %s, %s, %s, %s\}" % ((NUM,) * 12), body):
        v = [int(x, 0) for x in m.groups()]
        if v[4] == 0:
            break
        slots.append(v)
    # its texts: LANGUAGE + 0xE8 (field_stage.c, EU); the JSONs' textFile is the English one, LANGUAGE 2
    return {"name": "WSTAG260 (FIELDSTG's own field)", "modes": [[0x210, 0, 0, 0]], "euModes": [[0x210, 0, 0, 0]],
            "textFile": 0xE8 + 2, "slots": slots, "actors": actors, "places": [], "events": [], "virtual": VERSION}


def paths():
    """the virtual stages' JSONs, written again if the decomp's C is newer or VERSION changed"""
    OUT.mkdir(parents=True, exist_ok=True)
    p = OUT / "wstag260.json"
    srcs = [DECOMP / "src" / "fieldstg" / "data" / "fieldstg.c", Path(__file__)]
    stale = not p.exists() or any(s.stat().st_mtime > p.stat().st_mtime for s in srcs)
    if not stale:
        try:
            stale = json.loads(p.read_text()).get("virtual") != VERSION
        except ValueError:
            stale = True
    if stale:
        p.write_text(json.dumps(field_stage()), encoding="utf-8")
    return [p]


if __name__ == "__main__":
    d = field_stage()
    print(f"wstag260: {len(d['actors'])} actors, {sum(len(a['talks']) for a in d['actors'])} talks, {len(d['slots'])} slots")
