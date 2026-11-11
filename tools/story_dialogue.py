"""Lo que se dice en cada evento de un mapa: sus frases (las instrucciones 0x200 de su guion) en español, de la tabla
de textos del evento (EVENT_TEXT(n) del archivo EVENT_TEXT_FILE del mapa, el europeo + 6 para el español). Para
escribir los pasos de la historia en el Digilog.
Uso: python native/tools/story_dialogue.py EVENTO [EVENTO ...]   (números de evento, decimales)
"""
import re, struct, sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
import quest_talks as q  # noqa: E402

STAGES = q.NATIVE.parent / "dw3_decomp" / "src" / "stages"
FILES = q.DATA / "files"
SPANISH = 6  # LANGUAGE: TEXT_FILE(1) = LANGUAGE

def archive_entry(a, i):
    count = 0
    k = 0
    while count == 0 and k + 4 <= len(a):
        count = struct.unpack_from("<I", a, k)[0] // 4
        k += 4
    if i >= count:
        return None
    off = struct.unpack_from("<I", a, i * 4)[0]
    nxt = struct.unpack_from("<I", a, (i + 1) * 4)[0] if i + 1 < count else len(a)
    return a[off:nxt if nxt > off else len(a)]

def find(event, only=None):
    for f in STAGES.rglob("wstag*.c"):
        if only and f.stem != only:
            continue
        src = f.read_text(encoding="utf-8", errors="replace")
        m = re.search(r"\{\s*%d,\s*(\w+),\s*(EVENT_TEXT\((0x[0-9A-Fa-f]+|\d+)\)|0|NULL)" % event, src)
        if not m:
            continue
        eu = re.findall(r"#define EVENT_TEXT_FILE (0x[0-9A-Fa-f]+)", src)
        script = re.search(r"s16 %s\[\] = \{(.*?)\};" % m.group(1), src, re.S)
        return f.stem, int(eu[-1], 16) if eu else None, int(m.group(3), 0) if m.group(3) else None, script.group(1) if script else ""
    return None

for arg in sys.argv[1:]:
    only, _, ev = arg.rpartition(":")  # "wstag480:171", o solo el número
    ev = int(ev)
    r = find(ev, only or None)
    if not r:
        print(f"evento {ev}: no encontrado")
        continue
    stage, base, n, script = r
    texts = [int(x, 0) for x in re.findall(r"0x200,\s*\w+,\s*(0x[0-9A-Fa-f]+|\d+),", script)]
    print(f"== evento {ev} ({stage}): {len(texts)} frases")
    if base is None or n is None:
        continue
    table = archive_entry((FILES / ("%04d.bin" % (base + SPANISH))).read_bytes(), n)
    if not table:
        continue
    for t in (texts if len(texts) <= 5 else texts[:3] + texts[-2:]):  # las primeras y las últimas
        try:
            print("   ", q.dec(q.entry(table, t, 400), True)[:150])
        except Exception:
            pass
