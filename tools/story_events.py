"""Los eventos de la historia por paso: en el código de cada mapa (dw3_decomp/src/stages), el switch de GAME.progress
lanza, en orden, el primer evento que no se ha visto (su marca F40:xx a 0). Por cada paso de la historia, esa cadena
de eventos (mapa, marca y lo que hace, del comentario de su endEventN) son los pasos dentro del paso.
Uso: python native/tools/story_events.py [paso_hex ...]
"""
import re, sys, collections
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
STAGES = NATIVE.parent / "dw3_decomp" / "src" / "stages"
PLACES = dict(re.findall(r"- `(wstag\d+)`: (.*)", (STAGES / "README.md").read_text(encoding="utf-8")))

def num(s):
    return int(s, 0)

by_step = collections.defaultdict(list)
JUMPS = {}  # (mapa, evento): los modos a los que salta su guion
for f in sorted(STAGES.rglob("wstag*.c")):
    if f.stem.startswith("wstag9") or f.stem == "wstag586":
        continue
    src = f.read_text(encoding="utf-8", errors="replace")
    # lo que hace cada evento al acabar: el comentario de su endEventN
    ends = {}
    for m in re.finditer(r"/\*\s*(After events? (\d+)(?:[^*]|\*(?!/))*)\*/", src):
        ends.setdefault(int(m.group(2)), " ".join(m.group(1).replace("*", " ").split()))
    # cada evento lanzado: su paso de la historia y su marca, de lo más cercano por encima dentro de su función (un
    # `case X:` de un switch de GAME.progress, o un `GAME.progress == X` en su condición)
    lines = src.split("\n")
    for n, l in enumerate(lines):
        for e in re.finditer(r"FIELDSTG_startEvent\((0x[0-9A-Fa-f]+|\d+)\)", l):
            ev = num(e.group(1))
            steps, flag = [], ""
            for back in range(n, max(n - 40, -1), -1):
                t = lines[back]
                if back < n and re.match(r"^\w.*\(.*\)\s*\{\s*$", t):  # el principio de la función
                    break
                if not flag:
                    m = re.search(r"FLAG\(0x40, (0x[0-9A-Fa-f]+|\d+)\)", t)
                    if m:
                        flag = f"F40:{num(m.group(1)):02x}"
                m = re.findall(r"GAME\.progress == (0x[0-9A-Fa-f]+|\d+)", t) or re.findall(r"case (0x[0-9A-Fa-f]+|\d+):", t)
                if m:
                    steps = [num(x) for x in m]
                    break
            for p in steps or [-1]:
                by_step[p].append((f.stem, flag, ev, ends.get(ev, "")))
    # los que lanza pisar un sitio (SLOT_EVENT de stageSlots): su paso, PROGRESS(x), y sus marcas en las condiciones
    for m in re.finditer(r"\{ \{ \{ ([^}]*) \}, \{ ([^}]*) \} \}, SLOT_EVENT, (0x[0-9A-Fa-f]+|\d+),", src):
        ev, conds, steps, flag = num(m.group(3)), [], [], ""
        for c in (m.group(1), m.group(2)):
            pm = re.match(r"PROGRESS\((0x[0-9A-Fa-f]+|\d+)\), 1", c)
            fm = re.match(r"FLAG\((0x[0-9A-Fa-f]+|\d+), (0x[0-9A-Fa-f]+|\d+)\), (\d+)", c)
            if pm:
                steps.append(num(pm.group(1)))
            elif fm:
                g, k, v = num(fm.group(1)), num(fm.group(2)), fm.group(3)
                if g == 0x40 and v == "0" and not flag:
                    flag = f"F40:{k:02x}"
                else:
                    conds.append(f"F{g:02X}:{k:02X}={v}")
            elif not c.startswith("CODES_END"):
                conds.append(c.replace(" ", ""))
        what = ("al pisar un sitio" + (f" si {', '.join(conds)}" if conds else "") + "; " + ends.get(ev, "")).strip("; ")
        for p in steps or [-1]:
            by_step[p].append((f.stem, flag, ev, what))
    # adónde lleva cada evento: los saltos de mapa de su guion (0x304, modo, x, y, dir)
    scripts = dict(re.findall(r"\{\s*(\d+),\s*(script\w+),", src))
    for st_ev in [x for p in by_step for x in by_step[p] if x[0] == f.stem]:
        name = scripts.get(str(st_ev[2]))
        if not name:
            continue
        body = re.search(r"s16 %s\[\] = \{(.*?)\};" % name, src, re.S)
        if body:
            for j in re.finditer(r"0x304,\s*(0x[0-9A-Fa-f]+|\d+),\s*(0x[0-9A-Fa-f]+|\d+),\s*(0x[0-9A-Fa-f]+|\d+),", body.group(1)):
                JUMPS.setdefault((f.stem, st_ev[2]), []).append(f"{num(j.group(1)):#x} ({num(j.group(2))}, {num(j.group(3))})")

want = {int(x, 16) for x in sys.argv[1:]}
for p in sorted(by_step):
    if want and p not in want:
        continue
    print(f"######## historia {p:#x}")
    for stage, flag, ev, what in by_step[p]:
        jump = JUMPS.get((stage, ev))
        jump = f" → lleva a {', '.join(dict.fromkeys(jump))}" if jump else ""
        print(f"  {stage} ({PLACES.get(stage, '')[:40]}) evento {ev} {flag}: {what[:170]}{jump}")
