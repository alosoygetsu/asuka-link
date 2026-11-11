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

want = {int(x, 16) for x in sys.argv[1:]}
for p in sorted(by_step):
    if want and p not in want:
        continue
    print(f"######## historia {p:#x}")
    for stage, flag, ev, what in by_step[p]:
        print(f"  {stage} ({PLACES.get(stage, '')[:40]}) evento {ev} {flag}: {what[:170]}")
