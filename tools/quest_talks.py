"""Vuelca las conversaciones de los mapas que se le pidan (por modo, en hexadecimal), con sus condiciones, sus acciones
(objetos, marcas, combates, eventos) y lo que dicen en español: para estudiar las misiones del Digilog paso a paso.
Lee el juego extraído (native/build/data). Uso: python native/tools/quest_talks.py 233 23c 2b8 ...
"""
import json, glob, struct, os, re, sys
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
DATA = NATIVE / "build" / "data"

def dec(b, talk):
    marks = {0x02: ' ', 0x03: ' | ', 0x04: ',', 0x05: '.', 0x13: "'", 0x51: 'é', 0x5f: 'á', 0x60: 'í', 0x61: 'ó', 0x62: 'ú',
             0x63: 'ñ', 0xe6: '?', 0xe7: ''} if talk else {0x61: 'ó'}
    s = ''
    for c in b:
        if c == 0: break
        if c == 1: s += ' '
        elif 0x0E <= c <= 0x27: s += chr(65 + c - 0x0E)
        elif 0x28 <= c <= 0x41: s += chr(97 + c - 0x28)
        elif not talk and 0x04 <= c <= 0x0D: s += chr(48 + c - 0x04)
        elif c in marks: s += marks[c]
    return re.sub(r'[ ]+', ' ', s).strip()

def entry(data, i, n=700):
    o = struct.unpack_from('<I', data, 4 * (i + 1))[0]
    return data[o:o + n]

ITEMS = (DATA / "files" / "0112.bin").read_bytes()  # nombres de objetos, en español
def item(i): return dec(entry(ITEMS, i, 40), False)

# las condiciones especiales (SPECIAL), de la tabla europea de dw3_decomp (events.c)
DECOMP = NATIVE.parent / "dw3_decomp"
_ev = (DECOMP / "src/main/game/events.c").read_text(encoding="utf-8")
_eu = _ev[_ev.index("u8 SPECIAL_CONDITIONS[] = {"):]
_eu = _eu[_eu.index("#elif VERSION_EU"):_eu.index("#endif")]
_nums = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]+)", _eu)]
SPECIALS = {_nums[i]: (_nums[i + 1], _nums[i + 2]) for i in range(0, len(_nums) - 2, 3) if _nums[i] != 0xFF}
_rg = _ev[_ev.index("u8 PROGRESS_RANGES[][2] = {"):]
_rg = [int(x, 16) for x in re.findall(r"0x([0-9A-Fa-f]+)", _rg[:_rg.index("};")])]
RANGES = [(_rg[i], _rg[i + 1]) for i in range(0, len(_rg), 2)]
PARTNERS = ["Kotemon", "Kumamon", "Monmon", "Agumon", "Veemon", "Guilmon", "Renamon", "Patamon"]
SETS = {0: "Pez Rojo + Tela de Araña + Jabalina de Bambú", 1: "las 5 armas finales (Muramasa, Super Nova, Castigo, Eternamente, Invencible)"}

def special(i, v):
    if i not in SPECIALS:
        return f"esp{i}={v}"
    k, arg = SPECIALS[i]
    kind, op = k & 0xF0, k & 0xF
    n = "no " if v == 0 else ""
    if kind == 0x00: return f"{n}tiene {SETS.get(op, f'conjunto {op}')}"
    if kind == 0x10:
        p = PARTNERS[arg] if arg < 8 else str(arg)
        return {0: f"{n}empezó con el pack {arg}", 1: f"{n}tiene a {p}", 2: f"{n}lleva a {p} en el equipo", 3: f"desbloquea a {p}",
                4: f"{n}{p} a nivel 45", 5: f"{n}suma de niveles del equipo >= {arg * 15 + 30}", 6: f"{n}le falta {p}",
                7: f"{n}todos a nivel 45", 8: "cura al equipo", 9: f"{n}tiene a los 8"}.get(op, f"compañero op{op} {arg}")
    if kind == 0x20: return f"{n}dinero (op{op} {arg})"
    if kind == 0x30: return f"{n}historia entre {RANGES[arg][0]:#x} y {RANGES[arg][1]:#x}"
    if kind == 0x40: return f"{n}recuento de marcas {op}/{arg}"
    if kind == 0x50: return f"icono {arg}"
    return f"esp{i}({k:#x},{arg})={v}"

def code(c, v):
    if c == 0xFFFF: return None
    if c & 0x8000 and c < 0x9000: return f"{'+' if v else '-'}{item(c & 0x1FF)}"
    if 0x9000 <= c < 0x9200: return f"EVENTO {c & 0x1FF}"
    if 0x9200 <= c < 0x9400: return f"CARTA {c & 0x1FF}={v}"
    if 0x7400 <= c < 0x7600: return f"COMBATE {c & 0x1FF}"
    if 0x7600 <= c < 0x7A00: return f"DUELO {c:#x}"
    if 0x6000 <= c < 0x7000: return f"historia{'=' if v else '!='}{c & 0xFFF:#x}"
    if 0x7000 <= c < 0x7200: return special(c & 0x1FF, v)
    if 0x7200 <= c < 0x7400: return f"equipo{c & 0x1FF}={v}"
    if c < 0x6000: return f"F{c >> 8:02x}:{c & 0xFF:02x}={v}"
    return f"{c:#x}={v}"

def main():
    # --busca CÓDIGO[,CÓDIGO...]: en todos los mapas, las conversaciones cuyas acciones tocan esos códigos (hex: una marca
    # como 0x0601, un objeto como 0x8063), en lugar de las de unos modos
    global SEARCH
    SEARCH = set()
    # (con modos, solo en esos modos)
    args = sys.argv[1:]
    if "--busca" in args:
        i = args.index("--busca")
        SEARCH = {int(x, 16) for x in args[i + 1].split(",")}
        del args[i:i + 2]
    want = {int(m, 16) for m in args}
    for f in sorted(glob.glob(str(DATA / "stages" / "wstag*.json"))):
        d = json.load(open(f))
        modes = [m[0] for m in d.get("euModes") or d.get("modes") or []]
        if want and not want & set(modes) or not SEARCH and not want:
            continue
        text = (DATA / "files" / ("%04d.bin" % (d["textFile"] + 4))).read_bytes()
        print(f"==== {Path(f).stem} modos {[hex(m) for m in modes]}")
        # personaje N: Actor.key1 (el que usa probar.ps1 talk:N); actorI: su índice, el actorI del wstag*.c. El mismo
        # personaje sale en varios actorI (uno por condición); cada charla, una vez, con los actorI que la tienen
        found = {}
        for ai, a in enumerate(d.get("actors", [])):
            ac = [code(*c) for c in a.get("conditions", []) if code(*c)]
            for ti, t in enumerate(a.get("talks", [])):
                tc = [code(*c) for c in t.get("conditions", []) if code(*c)]
                ta = [code(*c) for c in t.get("actions", []) if code(*c)]
                if SEARCH and not any((x[0] & 0xF1FF if x[0] & 0x8000 and x[0] < 0x9000 else x[0]) in SEARCH or x[0] in SEARCH for x in t.get("actions", [])):
                    continue
                if not ta:  # solo lo que hace algo
                    continue
                key = (a["id"], t["entry"], tuple(sorted(tc)), tuple(ta))
                found.setdefault(key, (ac, []))[1].append(f"actor{ai}Talk{ti}")
        for (who, ent, tc, ta), (ac, where) in found.items():
            said = dec(entry(text, ent), True)[:160]
            print(f"  personaje {who} ({who:#x}) {','.join(where)} [{', '.join(ac)}] talk {ent} si [{', '.join(tc)}] -> [{', '.join(ta)}]")
            print(f"      «{said}»")


if __name__ == "__main__":
    main()
