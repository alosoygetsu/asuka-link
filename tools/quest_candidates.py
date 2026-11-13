# Misiones secundarias candidatas: las conversaciones que dan un objeto, sin el mapa de pruebas ni el capítulo extra,
# agrupadas por recompensa y clasificadas (herrero, encargo, regalo), con el lugar y lo que dice el personaje.
import json, glob, struct, os, re, collections

DATA = r"D:\Proyectos\dw2003-coop\native\build\data"
README = r"D:\Proyectos\dw2003-coop\dw3_decomp\src\stages\README.md"
KNOWN = {0x006, 0x022, 0x027, 0x026, 0x013, 0x168, 0x18B, 0x023, 0x066, 0x074, 0x099, 0x08D, 0x0DD, 0x088, 0x0A3, 0x030,
         0x033, 0x141, 0x18C, 0x004, 0x192, 0x02A, 0x0F2, 0x0F5, 0x100, 0x10E, 0x142, 0x0CB, 0x0F0}
SKIP_STAGES = {"wstag586"}  # el mapa de pruebas: lo da todo

ITEMCH = {0x01: ' '}
def dec_item(b):
    s = ''
    for c in b:
        if c == 0: break
        if c == 1: s += ' '
        elif 0x0E <= c <= 0x27: s += chr(65 + c - 0x0E)
        elif 0x28 <= c <= 0x41: s += chr(97 + c - 0x28)
        elif 0x04 <= c <= 0x0D: s += chr(48 + c - 0x04)
        elif c == 0x61: s += 'ó'
        else: s += '?'
    return ' '.join(s.split())

# los textos de diálogo: misma tabla de letras; signos y acentos aproximados
TALK = {0x02: ' ', 0x03: ' ', 0x04: ',', 0x05: '.', 0x13: "'", 0x1a: '?', 0x51: 'é', 0x5f: 'á', 0x60: 'í', 0x61: 'ó',
        0x62: 'ú', 0x63: 'ñ', 0xe6: '?', 0xe7: '', 0x5b: '¡', 0x5c: '¿'}
def dec_talk(b):
    s = ''
    for c in b:
        if c == 0: break
        if c == 1: s += ' '
        elif 0x0E <= c <= 0x27: s += chr(65 + c - 0x0E)
        elif 0x28 <= c <= 0x41: s += chr(97 + c - 0x28)
        elif c in TALK: s += TALK[c]
        else: s += ''
    return re.sub(r'\s+', ' ', s).strip()

def entry(data, i):
    o = struct.unpack_from('<I', data, 4 * (i + 1))[0]
    return data[o:o + 600]

names = open(os.path.join(DATA, 'files', '0112.bin'), 'rb').read()  # nombres de objetos en español
names_en = open(os.path.join(DATA, 'files', '0108.bin'), 'rb').read()
def item(i): return f"{dec_item(entry(names_en, i)[:40])} / {dec_item(entry(names, i)[:40])}"

places = {}
for line in open(README, encoding='utf-8'):
    m = re.match(r"- `(wstag\d+)`: (.*)", line)
    if m: places[m.group(1)] = m.group(2)

def cond_text(c, v):
    if c == 0xFFFF: return None
    if c & 0x8000 and c < 0x9000:
        i = c & 0x1FF
        return f"{'tiene' if v else 'no tiene'} {dec_item(entry(names, i)[:40])}"
    if 0x9000 <= c < 0x9200: return f"evento {c & 0x1FF}"
    if 0x7400 <= c < 0x7600: return f"combate {c & 0x1FF}"
    if 0x7600 <= c < 0x7A00: return f"duelo de cartas {c:#x}"
    if 0x6000 <= c < 0x7000: return f"historia en el paso {c & 0xFFF}"
    if 0x7000 <= c < 0x7200: return f"especial {c & 0x1FF}={v}"
    if 0x7200 <= c < 0x7400:
        import sys
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import quest_talks
        return quest_talks.party_stat(c & 0x1FF, v)
    if c < 0x6000: return f"marca {c >> 8:#x}:{c & 0xFF:#x}={v}"
    return f"{c:#x}={v}"

groups = collections.defaultdict(list)
for f in sorted(glob.glob(os.path.join(DATA, 'stages', 'wstag*.json'))):
    d = json.load(open(f))
    stage = os.path.basename(f)[:-5]
    if stage in SKIP_STAGES or stage.startswith('wstag9'):
        continue
    text = open(os.path.join(DATA, 'files', '%04d.bin' % (d['textFile'] + 4)), 'rb').read()  # +4: español
    modes = [m[0] for m in d.get('euModes') or d.get('modes') or []]
    for a in d.get('actors', []):
        for t in a.get('talks', []):
            acts = t.get('actions', [])
            gives = [x for x in acts if x[0] & 0x8000 and x[0] < 0x9000 and x[1] != 0]
            if not gives:
                continue
            takes = [x for x in acts if x[0] & 0x8000 and x[0] < 0x9000 and x[1] == 0]
            conds = [x for x in a.get('conditions', []) + t.get('conditions', [])]
            needs = [x for x in conds if x[0] & 0x8000 and x[0] < 0x9000 and x[1] == 1]
            flags = [x for x in conds if x[0] < 0x6000 and x[0] >> 8 != 0]
            kind = "herrero" if takes else "encargo" if (needs or flags) else "regalo"
            try:
                said = dec_talk(entry(text, t['entry']))
            except Exception:
                said = ''
            for g in gives:
                groups[g[0] & 0x1FF].append(dict(stage=stage, place=places.get(stage, stage), modes=modes, kind=kind,
                                                  conds=[cond_text(*c) for c in conds if cond_text(*c)],
                                                  acts=[cond_text(*x) for x in acts if cond_text(*x)], said=said[:260]))

order = {"herrero": 0, "encargo": 1, "regalo": 2}
rows = sorted(groups.items(), key=lambda kv: (order[kv[1][0]['kind']], kv[0]))
print(f"# Misiones candidatas para el Digilog\n\n{len(rows)} objetos que da un personaje (sin el mapa de pruebas ni el capítulo extra).\n")
for i, talks in rows:
    t = talks[0]
    known = " (ya en el Digilog)" if i in KNOWN else ""
    print(f"## {item(i)}{known}\n- **Tipo:** {t['kind']}\n- **Dónde:** {t['place']} (modo {', '.join(hex(m) for m in t['modes'])})")
    print(f"- **Condiciones:** {'; '.join(t['conds']) or 'ninguna'}\n- **Al hablar:** {'; '.join(t['acts'])}")
    if t['said']:
        print(f"- **Dice:** «{t['said']}»")
    if len(talks) > 1:
        print(f"- {len(talks) - 1} conversación(es) más con el mismo premio")
    print()
