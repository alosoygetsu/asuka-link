"""Los pasos dentro de cada paso de la historia (GAME.progress): por cada valor, las conversaciones que solo ocurren en
él (condición "historia = n") y hacen algo, con sus marcas, objetos, combates y eventos, el mapa y lo que dicen. Para
el Digilog: qué hay que hacer dentro de cada paso. Uso: python native/tools/story_steps.py [paso_hex ...]
"""
import json, glob, sys, collections
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import quest_talks as q  # noqa: E402  (decodifica códigos y textos)

DATA = q.DATA
want = {int(x, 16) for x in sys.argv[1:]}
by_step = collections.defaultdict(list)
for f in sorted(glob.glob(str(DATA / "stages" / "wstag*.json"))):
    d = json.load(open(f))
    stage = Path(f).stem
    if stage == "wstag586" or stage.startswith("wstag9"):
        continue
    modes = [m[0] for m in d.get("euModes") or d.get("modes") or []]
    text = (DATA / "files" / ("%04d.bin" % (d["textFile"] + 4))).read_bytes()
    seen = set()
    for a in d.get("actors", []):
        for t in a.get("talks", []):
            conds = a.get("conditions", []) + t.get("conditions", [])
            steps = [c[0] & 0xFFF for c in conds if 0x6000 <= c[0] < 0x7000 and c[1] == 1]
            if not steps or not t.get("actions"):
                continue
            key = (t["entry"], tuple(map(tuple, t["actions"])))
            if key in seen:
                continue
            seen.add(key)
            for p in set(steps):
                by_step[p].append((stage, modes, a["id"], t, conds, q.dec(q.entry(text, t["entry"]), True)[:150]))

for p in sorted(by_step):
    if want and p not in want:
        continue
    print(f"######## historia {p:#x}")
    for stage, modes, actor, t, conds, said in by_step[p]:
        c = [q.code(*x) for x in conds if q.code(*x) and not (0x6000 <= x[0] < 0x7000)]
        a = [q.code(*x) for x in t["actions"] if q.code(*x)]
        print(f"  {stage} {[hex(m) for m in modes][:2]} actor {actor} si [{', '.join(c)}] -> [{', '.join(a)}]")
        print(f"      «{said}»")
