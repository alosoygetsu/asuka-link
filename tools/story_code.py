"""El código de los mapas (dw3_decomp/src/stages) que hace avanzar la historia: por cada mapa, dónde cambia
GAME.progress y qué marcas activa (applyAction), con el comentario de la función que lo hace. Junto con
story_steps.py (las conversaciones), da los pasos dentro de cada paso de la historia.
Uso: python native/tools/story_code.py [paso_hex ...]  (sin argumentos, todo)
"""
import re, sys
from pathlib import Path

NATIVE = Path(__file__).resolve().parents[1]
STAGES = NATIVE.parent / "dw3_decomp" / "src" / "stages"
README = (STAGES / "README.md").read_text(encoding="utf-8")
PLACES = dict(re.findall(r"- `(wstag\d+)`: (.*)", README))

def code_text(c):
    c = int(c, 0) if isinstance(c, str) else c
    if c < 0x6000: return f"F{c >> 8:02x}:{c & 0xFF:02x}"
    if 0x8000 <= c < 0x9000: return f"objeto {c & 0x1FF:#x}"
    return f"{c:#x}"

want = {int(x, 16) for x in sys.argv[1:]}
for f in sorted(STAGES.rglob("wstag*.c")):
    lines = f.read_text(encoding="utf-8", errors="replace").split("\n")
    comment, func, hits = "", "", []
    for n, l in enumerate(lines):
        m = re.match(r"/\*+ ?(.*)", l)
        if m and not l.startswith(" "):
            block = [m.group(1)]
            k = n + 1
            while "*/" not in lines[k - 1] and k < len(lines):
                block.append(lines[k].strip(" *"))
                k += 1
            comment = " ".join(x for x in block if x).replace("*/", "").strip()
        m = re.match(r"^\w[\w\s\*]*\b(\w+)\(", l)
        if m and l.rstrip().endswith("{"):
            func = m.group(1)
        m = re.search(r"GAME\.progress = (0x[0-9A-Fa-f]+|\d+|\w+);", l)
        if m:
            p = m.group(1)
            hits.append((func, comment, f"HISTORIA -> {p}", int(p, 0) if p[0].isdigit() else None))
        for m in re.finditer(r"applyAction\((0x[0-9A-Fa-f]+|\d+|[A-Z_]+\([^)]*\)), ?(\w+)\)", l):
            c = m.group(1)
            try:
                cv = int(c, 0)
                txt = code_text(cv)
            except ValueError:
                txt = c
            hits.append((func, comment, f"{txt}={m.group(2)}", None))
    if not hits:
        continue
    if want and not any(h[3] in want for h in hits):
        continue
    print(f"==== {f.stem}: {PLACES.get(f.stem, '')}")
    last = None
    for func, comment, what, _ in hits:
        if func != last:
            print(f"  {func}: {comment[:200]}")
            last = func
        print(f"      {what}")
