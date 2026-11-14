#!/usr/bin/env python3
"""Consultas rápidas sobre el juego y el código, con salida corta (ruta:línea), para gastar poco contexto.

Uso: python native/tools/dw3.py                 (lista los subcomandos)
     python native/tools/dw3.py SUBCOMANDO ... [--max N]   (--max 0: sin límite)

Lee la decompilación (dw3_decomp: config/eu/*.yaml y símbolos, src/), el juego extraído (native/build/data, o
test2/data si falta) y el código del port (native/). Los subcomandos de historia y misiones llaman a los scripts
story_*.py, quest_*.py y journal_steps.py, sin duplicarlos.
"""
import contextlib
import functools
import io
import json
import re
import runpy
import struct
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
NATIVE = TOOLS.parent
ROOT = NATIVE.parent
DECOMP = ROOT / "dw3_decomp"
CFG = DECOMP / "config" / "eu"
sys.path.insert(0, str(TOOLS))

MAX = 20  # líneas por defecto; --max lo cambia


def rel(p, line=None):
    try:
        s = Path(p).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        s = Path(p).as_posix()
    return f"{s}:{line}" if line else s


def num(s):
    return int(s, 0) if isinstance(s, str) else s


def capped(lines, limit=None, more="--max N para ver más"):
    limit = MAX if limit is None else limit
    lines = list(lines)
    if limit and len(lines) > limit:
        return lines[:limit] + [f"… (+{len(lines) - limit}; {more})"]
    return lines


@functools.lru_cache(None)
def data_dir():
    for d in (NATIVE / "build" / "data", ROOT / "test2" / "data"):
        if (d / "SLES_039.36").exists():
            return d
    sys.exit("no encuentro el juego extraído (native/build/data o test2/data)")


@functools.lru_cache(None)
def file_names():
    return (data_dir() / "names.txt").read_text().split()


def game_file(name_or_index):
    i = file_names().index(name_or_index) if isinstance(name_or_index, str) else name_or_index
    return (data_dir() / "files" / f"{i:04d}.bin").read_bytes()


@functools.lru_cache(None)
def read(p):
    return Path(p).read_text(encoding="utf-8", errors="replace")


# ---------------------------------------------------------------- símbolos y overlays (addr, sym)

SYM_RE = re.compile(r"^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;(.*)")


@functools.lru_cache(None)
def symbols_of(path):
    """[(addr, name, size, is_func, line)] de un archivo de símbolos de splat"""
    out = []
    p = Path(path)
    if not p.exists():
        return out
    for n, l in enumerate(read(p).split("\n"), 1):
        m = SYM_RE.match(l)
        if m:
            size = re.search(r"size:(0x[0-9A-Fa-f]+|\d+)", m.group(3))
            out.append((int(m.group(2), 16), m.group(1), int(size.group(1), 0) if size else 0,
                        "type:func" in m.group(3), n, p))
    return out


@functools.lru_cache(None)
def stage_vram():
    m = re.search(r"STAGE_VRAM := (0x[0-9A-Fa-f]+)", read(DECOMP / "mk" / "version" / "eu.mk"))
    return int(m.group(1), 16)


@functools.lru_cache(None)
def overlays():
    """cada módulo cargable: dict(name, vram, end, subs=[(vram, kind, name)], syms=[archivos], src=dir)"""
    out = []
    for y in sorted(CFG.glob("*.yaml")):
        t = read(y)
        seg = re.search(r"type: code\n(.*?)(?=\n  - name:|\n  - \[0x[0-9A-Fa-f]+\]\s*$|\Z)", t, re.S)
        if not seg:
            continue
        body = seg.group(1)
        start = int(re.search(r"start: (0x[0-9A-Fa-f]+)", body).group(1), 16)
        vram = int(re.search(r"vram: (0x[0-9A-Fa-f]+)", body).group(1), 16)
        subs = []
        for m in re.finditer(r"^\s*- \[(0x[0-9A-Fa-f]+), \.?(\w+)(?:, ([\w/]+))?\]", body, re.M):
            subs.append((vram + int(m.group(1), 16) - start, m.group(2), m.group(3) or ""))
        for m in re.finditer(r"^\s*- \{ start: (0x[0-9A-Fa-f]+), type: \.?(\w+), vram: (0x[0-9A-Fa-f]+), name: ([\w/]+)", body, re.M):
            subs.append((int(m.group(3), 16), m.group(2), m.group(4)))
        subs.sort()
        ends = re.findall(r"^  - \[(0x[0-9A-Fa-f]+)\]\s*$", t, re.M)
        bss = re.search(r"bss_size: (0x[0-9A-Fa-f]+)", body)
        name = y.stem
        if name == "main":
            end = 0x80082CB0  # OVERLAY_VRAM: los overlays empiezan tras el .bss del ejecutable
        else:
            end = vram + int(ends[-1], 16) - start + (int(bss.group(1), 16) if bss else 0) if ends else vram
        syms = [DECOMP / s for s in re.findall(r"^\s*- (config/eu/\S+\.txt)", t, re.M)]
        src = re.search(r"src_path: (\S+)", t).group(1)
        out.append(dict(name=name, vram=vram, end=end, subs=subs, syms=syms,
                        src=DECOMP / src))
    # los escenarios: config/eu/stages.txt, en STAGE_VRAM
    sv = stage_vram()
    for l in read(CFG / "stages.txt").split("\n"):
        m = re.match(r"(WSTAG\d+)\s+(\S+)(?:\s+(\S+))?", l)
        if not m:
            continue
        name = m.group(1).lower()
        try:
            size = len(game_file(m.group(1) + ".PRO"))
        except (ValueError, FileNotFoundError):
            size = 0
        c = next(iter(sorted((DECOMP / "src" / "stages").rglob(name + ".c"))), None)
        subs = []
        if m.group(2) != "data":
            subs = [(sv, "head", name), (sv + int(m.group(2), 16), "text", name), (sv + int(m.group(3), 16), "data", name)]
        else:
            subs = [(sv, "data", name)]
        out.append(dict(name=name, vram=sv, end=sv + size, subs=subs, stage=True, cfile=c,
                        syms=[CFG / "symbols.txt", CFG / "symbols_fieldstg.txt", CFG / "stages" / f"{name}.txt"],
                        src=c.parent if c else DECOMP / "src" / "stages"))
    return out


def module_file(ov, subname):
    if ov.get("stage"):
        return ov.get("cfile")
    for ext in (".c", ".s", ""):
        p = ov["src"] / (subname + ext)
        if p.is_file():
            return p
    return None


def def_line(name, files):
    """(archivo, línea) de la definición de name (función, dato, macro, tipo) en esos archivos"""
    pat = re.compile(r"^(?!\s)(?!extern\b)(?:[\w\s\*]*[\s\*])?%s\s*(\(|\[|=|\{|;)|^#define %s\b|^\}\s*%s\s*;"
                     % ((re.escape(name),) * 3))
    for f in files:
        if not f or not Path(f).is_file():
            continue
        for n, l in enumerate(read(f).split("\n"), 1):
            if pat.search(l) and not l.rstrip().endswith(");"):
                return f, n
    return None


def cmd_addr(args):
    """addr 0x80...: a qué overlay, archivo fuente, función/símbolo pertenece (--ovl NOMBRE filtra)"""
    only = None
    if "--ovl" in args:
        i = args.index("--ovl")
        only = args[i + 1].lower()
        del args[i:i + 2]
    if not args:
        sys.exit("uso: dw3.py addr 0x8003EF5C [--ovl fieldstg]")
    a = num(args[0])
    if a < 0x80000000:
        a |= 0x80000000 if a < 0x200000 else 0
    if 0x1F800000 <= a < 0x1F800400:
        print(f"{a:#x}: scratchpad de la PSX")
        return
    if a < 0x80010000:
        print(f"{a:#x}: RAM del núcleo (BIOS), antes del ejecutable")
        return
    hits = []
    for ov in overlays():
        if only and ov["name"] != only:
            continue
        if not ov["vram"] <= a < max(ov["end"], ov["vram"] + 1):
            continue
        sub = None
        for i, s in enumerate(ov["subs"]):
            if s[0] <= a:
                sub = s
        syms = [s for f in ov["syms"] for s in symbols_of(f) if ov["vram"] <= s[0] < ov["end"] and s[0] <= a]
        exact = next((s for s in sorted(syms, reverse=True) if s[0] <= a < s[0] + max(s[2], 1)), None)
        near = max(syms, default=None)
        sub_start = sub[0] if sub else ov["vram"]
        sym = exact or (near if near and near[0] >= sub_start else None)
        src = module_file(ov, sub[2]) if sub else None
        where = ""
        if sym:
            d = def_line(sym[1], [src] if src else [])
            if not d:
                d = (sym[5], sym[4])
            where = rel(*d)
        elif src:
            where = rel(src)
        label = f"{sym[1]}+{a - sym[0]:#x}" if sym and a != sym[0] else (sym[1] if sym else "?")
        kind = "func" if sym and sym[3] else ("dato" if sym else "")
        sec = f"[{sub[1]} {sub[2]}]" if sub else ""
        rank = (0 if exact else 1, 1 if ov.get("stage") else 0)
        hits.append((rank, f"{ov['name']:9} {label} {kind} {sec} {where}".replace("  ", " ").strip()))
    if not hits:
        print(f"{a:#x}: fuera de todo módulo conocido")
        return
    hits.sort(key=lambda h: h[0])
    if len(hits) > 1:
        print(f"{a:#x}: zona de overlays, {len(hits)} candidatos (cargan en la misma dirección; --ovl NOMBRE filtra)")
    else:
        print(f"{a:#x}:")
    for l in capped([h[1] for h in hits], more="--max N o --ovl NOMBRE"):
        print("  " + l)


SRC_ROOTS = [DECOMP / "src", DECOMP / "include", NATIVE / "coop", NATIVE / "platform", NATIVE / "launcher",
             NATIVE / "include"]
PATCHED = NATIVE / "build" / "patched"


@functools.lru_cache(None)
def source_files(with_patched=True):
    out = []
    for r in SRC_ROOTS:
        out += [p for p in r.rglob("*") if p.suffix in (".c", ".h") and p.is_file()]
    if with_patched:
        patched = {p.relative_to(NATIVE / "patches").with_suffix("") for p in (NATIVE / "patches").rglob("*.patch")}
        patched |= {p.relative_to(NATIVE / "coop" / "patches").with_suffix("") for p in (NATIVE / "coop" / "patches").rglob("*.patch")}
        out += [PATCHED / p for p in sorted(patched) if (PATCHED / p).is_file()]
    return out


def cmd_sym(args):
    """sym NOMBRE: dónde se define (decomp, parcheado, port) y quién lo usa"""
    if not args:
        sys.exit("uso: dw3.py sym NOMBRE")
    name = args[0]
    word = re.compile(r"\b%s\b" % re.escape(name))
    for f in sorted(CFG.rglob("*.txt")):
        for s in symbols_of(f):
            if s[1] == name:
                print(f"dirección {s[0]:#x}{' (func)' if s[3] else ''}{f' tamaño {s[2]:#x}' if s[2] else ''}  {rel(s[5], s[4])}")
    defs, uses = [], {}
    for f in source_files():
        t = read(f)
        if name not in t:
            continue
        d = def_line(name, [f])
        if d:
            defs.append(rel(*d) + ("  (parcheado)" if PATCHED in f.parents else ""))
        for n, l in enumerate(t.split("\n"), 1):
            if word.search(l) and (not d or n != d[1]):
                uses.setdefault(f, []).append(n)
    print("definido en: " + (", ".join(defs) if defs else "(no encontrado)"))
    lines = []
    stages = sorted(f for f in uses if f.stem.startswith("wstag"))
    if len(stages) > 3:  # los mapas, en una línea
        lines.append(f"  {len(stages)} mapas: " + " ".join(f.stem for f in stages[:12]) + (" …" if len(stages) > 12 else ""))
        uses = {f: ns for f, ns in uses.items() if f not in stages}
    for f, ns in sorted(uses.items(), key=lambda kv: (PATCHED in kv[0].parents, str(kv[0]))):
        fn = enclosing_function(f, ns[0])
        lines.append(f"  {rel(f, ns[0])}{f' {fn}()' if fn else ''}" + (f"  (+{len(ns) - 1} más)" if len(ns) > 1 else ""))
    print(f"usado en {len(uses)} archivos:")
    for l in capped(lines):
        print(l)


FUNC_RE = re.compile(r"^(?!static\s+const)(?:[\w\*]+\s+)+\**(\w+)\(([^;]*)$")


def enclosing_function(f, line):
    ls = read(f).split("\n")
    for k in range(line - 1, -1, -1):
        l = ls[k]
        if l and not l[0].isspace() and l[0] not in "#/{}*":
            m = FUNC_RE.match(l)
            if m:
                return m.group(1)
            m = re.match(r"^(?:static\s+)?(?:const\s+)?\w+\s+(\w+)\[\]", l)
            if m:
                return m.group(1)
            return None
    return None


# ---------------------------------------------------------------- textos (texto)

LANGS = "EFIDS"  # inglés, francés, italiano, alemán, español; M y U (japonés/USA) no van en el disco europeo
LANG_NAME = dict(E="en", F="fr", I="it", D="de", S="es")


@functools.lru_cache(None)
def inverse_table():
    import game_text
    return {v: k for k, v in game_text.learn().items()}


def decode(b):
    inv = inverse_table()
    s, p, name = [], 0, False
    while p < len(b) and b[p] != 0:
        n = 2 if b[p] in (1, 2) else 1
        c = bytes(b[p:p + n])
        if c == b"":  # el nombre de quien habla, entre dos 02 07
            s.append(": " if name else "")
            name = not name
        elif c == b"":  # fin de página
            s.append(" ")
        else:
            s.append(inv.get(c, " " if c[0] == 2 else ""))
        p += n
    return " ".join("".join(s).split())


def string_table(b):
    """los textos de un archivo de textos (un recuento, sus posiciones, los textos), o None"""
    if len(b) < 8:
        return None
    n = struct.unpack_from("<I", b, 0)[0]
    if not 0 < n < 5000 or 4 + 4 * n > len(b):
        return None
    offs = struct.unpack_from(f"<{n}I", b, 4)
    if any(o >= len(b) or o < 4 + 4 * n for o in offs if o):
        return None
    # cada texto acaba en un 0 o donde empieza el siguiente (los de los eventos no siempre llevan el 0)
    ends = sorted(set(offs) | {len(b)})
    return [decode(b[o:min(next(e for e in ends if e > o), o + 2000)]) for o in offs]


def archive(b):
    """un archivo de archivos (los textos de los eventos, ?SDMG*.BIN): sus partes"""
    if len(b) < 8:
        return None
    first = struct.unpack_from("<I", b, 0)[0]
    if first % 4 or not 4 <= first < len(b) or first > 4 * 2000:
        return None
    offs = list(struct.unpack_from(f"<{first // 4}I", b, 0))
    if offs != sorted(offs) or offs[-1] > len(b):
        return None
    return [b[o:(offs[i + 1] if i + 1 < len(offs) else len(b))] for i, o in enumerate(offs)]


TEXT_CACHE_VERSION = 3  # súbelo al cambiar decode()


@functools.lru_cache(None)
def text_index():
    """{(archivo sin idioma, parte|None, índice): {idioma: texto}} de todos los archivos de textos, en caché"""
    cache = NATIVE / "build" / "dw3_texts.json"
    names = file_names()
    stamp = (data_dir() / "names.txt").stat().st_mtime
    if cache.exists():
        try:
            c = json.loads(cache.read_text(encoding="utf-8"))
            if c["stamp"] == stamp and c["data"] == str(data_dir()) and c.get("v") == TEXT_CACHE_VERSION:
                return c["texts"]
        except (ValueError, KeyError):
            pass
    texts = []  # [archivo, parte, índice, idioma, texto]
    for i, nm in enumerate(names):
        if len(nm) < 3 or nm[0] not in LANGS or nm[1] != "S" or not nm.endswith(".BIN"):
            continue
        b = game_file(i)
        if "DMG" in nm:
            for part, sub in enumerate(archive(b) or []):
                for k, s in enumerate(string_table(sub) or []):
                    if s:
                        texts.append([nm[1:], part, k, nm[0], s])
        else:
            for k, s in enumerate(string_table(b) or []):
                if s:
                    texts.append([nm[1:], None, k, nm[0], s])
    try:
        cache.write_text(json.dumps({"v": TEXT_CACHE_VERSION, "stamp": stamp, "data": str(data_dir()), "texts": texts}, ensure_ascii=False),
                         encoding="utf-8")
    except OSError:
        pass
    return texts


@functools.lru_cache(None)
def stages_json():
    out = {}
    for f in sorted((data_dir() / "stages").glob("wstag*.json")):
        out[f.stem] = json.loads(f.read_text())
    return out


def talk_users(fname, idx):
    """los mapas y actores que dicen el texto idx de ?STALKnn.BIN"""
    names = file_names()
    try:
        en = names.index("E" + fname)
    except ValueError:
        return []
    out = []
    for st, d in stages_json().items():
        if d.get("textFile") != en:
            continue
        modes = [hex(m[0]) for m in d.get("euModes") or d.get("modes") or []][:2]
        for i, a in enumerate(d.get("actors", [])):
            for k, t in enumerate(a.get("talks", [])):
                if t.get("entry") == idx:
                    out.append(f"{st} {'/'.join(modes)} actor{i}Talk{k}")
    return out


def event_users(fname, part, idx):
    """los eventos cuyo guion dice el texto idx de la parte part de ?SDMGnnn.BIN"""
    names = file_names()
    try:
        base = names.index("M" + fname)
    except ValueError:
        return []
    out = []
    for f in (DECOMP / "src" / "stages").rglob("wstag*.c"):
        t = read(f)
        eu = re.findall(r"#define EVENT_TEXT_FILE (0x[0-9A-Fa-f]+)", t)
        if not eu or int(eu[-1], 16) != base:
            continue
        for m in re.finditer(r"\{\s*(\d+),\s*(\w+),\s*EVENT_TEXT\((0x[0-9A-Fa-f]+|\d+)\)", t):
            if int(m.group(3), 0) != part:
                continue
            script = re.search(r"s16 %s\[\] = \{(.*?)\};" % m.group(2), t, re.S)
            if script and re.search(r"0x200,\s*\w+,\s*(%d|%#x|%#X)," % (idx, idx, idx), script.group(1), re.I):
                line = t[:script.start()].count("\n") + 1
                out.append(f"evento {m.group(1)} {rel(f, line)}")
    return out


def snippet(s, q, width=110):
    s = s.removeprefix(": ")
    i = s.casefold().find(q)
    if len(s) <= width or i < 60:
        return s[:width] + ("…" if len(s) > width else "")
    return "…" + s[i - 40:i - 40 + width] + ("…" if i - 40 + width < len(s) else "")


def cmd_texto(args):
    """texto "frase": en qué archivo de textos, índice, mapa o evento sale, en los 5 idiomas"""
    if not args:
        sys.exit('uso: dw3.py texto "frase"')
    q = " ".join(args).casefold()
    texts = text_index()
    by_key = {}
    for fn, part, k, lang, s in texts:
        by_key.setdefault((fn, part, k), {})[lang] = s
    hits = []
    for fn, part, k, lang, s in texts:
        if q in s.casefold() and (fn, part, k) not in hits:
            hits.append((fn, part, k))
    print(f"{len(hits)} textos con «{' '.join(args)}»")
    shown = 0
    for key in hits:
        if MAX and shown >= MAX:
            print(f"… (+{len(hits) - shown}; --max N para ver más)")
            break
        fn, part, k = key
        where = f"?{fn}" + (f" parte {part}" if part is not None else "") + f" #{k}"
        users = talk_users(fn, k) if "TALK" in fn else event_users(fn, part, k) if part is not None else []
        print(f"- {where}" + (f"  ← {'; '.join(users[:4])}" + (f" (+{len(users) - 4})" if len(users) > 4 else "") if users else ""))
        tr = by_key[key]
        shown += 1
        if len(set(tr.values())) == 1:
            print(f"    todos: {snippet(next(iter(tr.values())), q)}")
            continue
        for lang in LANGS:
            if lang in tr:
                print(f"    {LANG_NAME[lang]}: {snippet(tr[lang], q)}")


# ---------------------------------------------------------------- marcas (marca)

def parse_flag(s):
    s = s.strip().upper().replace("F", "", 1) if s.upper().startswith("F") and ":" in s else s
    if ":" in s:
        g, n = s.split(":")
        return int(g, 16) << 8 | int(n, 16)
    return int(s, 16) if not s.lower().startswith("0x") else int(s, 16)


def flag_text(c):
    return f"F{c >> 8:02X}:{c & 0xFF:02X}"


FLAG_RE = re.compile(r"\b(FLAG|F)\(\s*(0x[0-9A-Fa-f]+|\d+)\s*,\s*(0x[0-9A-Fa-f]+|\d+)\s*\)(\s*,\s*(0x[0-9A-Fa-f]+|\d+))?")


@functools.lru_cache(None)
def talk_text(stage, actor, talk):
    d = stages_json().get(stage)
    try:
        import quest_talks as q
        entry = d["actors"][actor]["talks"][talk]["entry"]
        b = game_file(d["textFile"] + 4)  # el español
        return decode(q.entry(b, entry)).removeprefix(": ")[:70]
    except Exception:
        return ""


def cmd_marca(args):
    """marca F40:05: dónde se activa, se borra y se comprueba una marca (código, conversaciones, guiones, Digilog)"""
    if not args:
        sys.exit("uso: dw3.py marca F40:05   (o 0x4005, 40:05)")
    code = parse_flag(args[0])
    raw = re.compile(r"\b0x0*%X\b" % code, re.I)
    sets, tests, other, notes = [], [], [], []
    files = [p for p in source_files(with_patched=False) if p.name != "journal_steps.h"]
    for f in files:
        t = read(f)
        if "FLAG" not in t and "F(" not in t and not raw.search(t):
            continue
        ls = t.split("\n")
        for n, l in enumerate(ls, 1):
            val = None
            found = False
            for m in FLAG_RE.finditer(l):
                if m.group(1) == "F" and f.suffix != ".py":
                    continue
                if (int(m.group(2), 0) << 8 | int(m.group(3), 0)) == code:
                    found, val = True, m.group(5)
                    break
            if not found and raw.search(l) and (l.lstrip().startswith(("/*", "*", "//")) or "flag" in l.lower()):
                # el número suelto, en los comentarios ("sets flag 0x4005"); en el código suele ser otra cosa (un
                # opcode de guion como 0x601)
                notes.append(f"{rel(f, n)}  {l.strip(' /*').strip()[:90]}")
                continue
            if not found:
                continue
            where = rel(f, n)
            ctx = enclosing_function(f, n) or ""
            am = re.match(r"actor(\d+)Talk(\d+)(Conditions|Actions)", ctx)
            say = ""
            if am:
                say = talk_text(f.stem, int(am.group(1)), int(am.group(2)))
                say = f" «{say}»" if say else ""
            v = f"={int(val, 0)}" if val else ""
            if "applyAction" in l or (ctx.endswith("Actions")):
                sets.append(f"{'activa' if val != '0' else 'borra '}{v} {where} {ctx}{say}")
            elif "checkCondition" in l or ctx.endswith("Conditions") or re.search(r"\b(if|while)\b|==|!=", l):
                tests.append(f"comprueba{v} {where} {ctx}{say}")
            else:
                other.append(f"usa{v} {where} {ctx}  {l.strip()[:70]}")
    import journal_steps as j
    for steps_of, steps in j.STORY:
        for c, _, es, _ in steps:
            if c == code:
                other.append(f"Digilog historia {', '.join(hex(p) for p in steps_of)}: {es}")
    for item, steps in j.QUEST_STEPS.items():
        for c, _, es, _ in steps:
            if c == code:
                other.append(f"Digilog misión (objeto {item:#x}): {es}")
    if code >> 8 == 0x20:
        print(f"(grupo 0x20: el mapa del modo {0x200 | code & 0xFF:#x} ya visitado)")
    print(f"marca {flag_text(code)} (código {code:#06x}): {len(sets)} cambios, {len(tests)} comprobaciones, {len(other)} otros, {len(notes)} comentarios")
    for title, ls in (("se cambia:", sets), ("se comprueba:", tests), ("otros (tablas, Digilog):", other),
                      ("comentarios:", notes)):
        if ls:
            print(title)
            for l in capped(ls):
                print("  " + l)


# ---------------------------------------------------------------- Digimon, técnicas, digievoluciones

DIGI = 0x8003EF5C
TECHS = 0x80042BDC
EVOS = 0x8008903C  # en STFGTREP.PRO, vram 0x80082CB0: 8 punteros a 44 entradas de 16 bytes
ELEM = {2: "Fuego", 3: "Agua", 4: "Hielo", 5: "Viento", 6: "Rayo", 7: "Máquina", 8: "Oscuridad"}
STATS = {1: "Fuerza", 2: "Defensa", 3: "Espíritu", 4: "Sabiduría", 5: "Velocidad", 6: "Carisma", 7: "Nivel",
         8: "Res.Fuego", 9: "Res.Agua", 10: "Res.Hielo", 11: "Res.Viento", 12: "Res.Rayo", 13: "Res.Máquina",
         14: "Res.Oscuridad"}
DIGI_COUNT = 55


@functools.lru_cache(None)
def exe():
    return (data_dir() / "SLES_039.36").read_bytes()


def at(addr):
    return addr - struct.unpack_from("<I", exe(), 0x18)[0] + 0x800


@functools.lru_cache(None)
def names_of(kind, lang):
    return string_table(game_file(f"{lang}S{kind}.BIN"))


@functools.lru_cache(None)
def digimon_rows():
    rows = []
    e = exe()
    for i in range(DIGI_COUNT):
        o = at(DIGI) + i * 0x58
        rows.append(dict(i=i, id=struct.unpack_from("<H", e, o)[0], stats=struct.unpack_from("<6H", e, o + 2),
                         res=struct.unpack_from("<7H", e, o + 0x0E), skills=struct.unpack_from("<7H", e, o + 0x1C),
                         pair=struct.unpack_from("<H", e, o + 0x2A)[0], levels=list(e[o + 0x31:o + 0x37]),
                         known=list(e[o + 0x37:o + 0x3C]), pairp=e[o + 0x3D], hp=e[o + 0x3F], mp=e[o + 0x40],
                         blast=list(e[o + 0x50:o + 0x55]), name_id=e[o + 0x55]))
    return rows


def dname(i, lang="S"):
    """nombre del Digimon de índice i (desde 0)"""
    r = digimon_rows()[i]
    ns = names_of("DIGNAM", lang)
    return ns[r["name_id"]] if r["name_id"] < len(ns) else f"#{i}"


def tech(n):
    o = at(TECHS) + (n - 1) * 0x12
    mp, power, icon, kind, acc, element, epow, family, effect, echance, epower = struct.unpack_from("<HHBBBBBBBBB", exe(), o)
    return dict(mp=mp, power=power, kind=kind, acc=acc, element=element, effect=effect, echance=echance)


@functools.lru_cache(None)
def tech_effects():
    """TECH_EFFECT_* de include/fightstg/battle.h: {número: nombre}"""
    h = read(DECOMP / "include" / "fightstg" / "battle.h")
    return {int(v): k.lower() for k, v in re.findall(r"#define TECH_EFFECT_(\w+) (\d+)", h) if k != "FIRST"}


def tname(n, lang="S"):
    ns = names_of("SKLNAM", lang)
    return ns[n] if n < len(ns) else f"#{n}"


def tech_line(n):
    t = tech(n)
    eff = tech_effects().get(t["effect"]) if t["effect"] >= 2 else None
    return (f"{both(tname(n), tname(n, 'E'))} (#{n}): PM {t['mp']}, poder {t['power'] or '-'}, "
            f"{ELEM.get(t['element'], 'sin elemento')}{', cura' if t['kind'] == 3 else ''}, precisión {t['acc']}"
            + (f", efecto {eff or t['effect']} {t['echance']}%" if eff or t["effect"] >= 2 else ""))


def both(es, en):
    return es if es == en else f"{es} / {en}"


def find_digimon(q):
    q = q.casefold()
    rows = [r for r in digimon_rows() if r["id"]]
    exact = [r["i"] for r in rows if q in (dname(r["i"]).casefold(), dname(r["i"], "E").casefold())]
    return exact or [r["i"] for r in rows if q in dname(r["i"]).casefold() or q in dname(r["i"], "E").casefold()]


@functools.lru_cache(None)
def evolutions():
    """[(compañero, [(forma, [(forma necesaria, nivel)], (stat, valor)|None)])] de STFGTREP.PRO"""
    ovl = game_file("STFGTREP.PRO")
    vram = 0x80082CB0
    ptrs = struct.unpack_from("<8I", ovl, EVOS - vram)
    rows = digimon_rows()
    out = []
    for p in range(8):
        lst = []
        for n in range(44):
            d, n1, l1, n2, l2, st, val = struct.unpack_from("<i4hhh", ovl, ptrs[p] - vram + n * 16)
            if d <= 0 or rows[d - 1]["id"] <= 0:
                continue
            needs = [(a - 1, b) for a, b in ((n1, l1), (n2, l2)) if a]
            lst.append((d - 1, needs, (STATS.get(st, f"stat {st}"), val) if st else None))
        out.append((p, lst))
    return out


def evo_needs(needs, cond):
    parts = [f"{dname(a)} nv{lv}" for a, lv in needs]
    if cond:
        parts.append(f"nivel {cond[1]}" if cond[0] == "Nivel" else f"{cond[0]} ≥{cond[1]}")
    return " + ".join(parts) or "-"


def cmd_digimon(args):
    """digimon NOMBRE: datos, técnicas (con niveles) y digievoluciones de un Digimon"""
    if not args:
        for r in digimon_rows():
            if r["id"]:
                print(f"#{r['i']} {dname(r['i'])}", end="  ")
        print()
        return
    found = find_digimon(" ".join(args))
    if not found:
        sys.exit("ningún Digimon con ese nombre")
    for i in found[:3]:
        r = digimon_rows()[i]
        print(f"#{i} {both(dname(i), dname(i, 'E'))}  (id {r['id']:#x}, DIGIMON_DATA+{i * 0x58:#x} = {DIGI + i * 0x58:#x})")
        pv = f"PV {r['hp']} PM {r['mp']}  " if i < 8 else ""
        print(f"  {pv}stats (fue def esp sab vel car) {list(r['stats'])}  resist. (fue agu hie vie ray máq osc) {list(r['res'])}")
        sk = r["skills"]
        lv6 = f" (nv{r['levels'][5]})" if r["levels"][5] else ""
        print(f"  característica: {tname(sk[6])}{lv6}" + (f"  ataque básico #{sk[0]}" if sk[0] else ""))
        for k in range(1, 6):
            if sk[k]:
                print(f"  - {both(tname(sk[k]), tname(sk[k], 'E'))}: aprende nv{r['levels'][k - 1]}, carga nv{r['known'][k - 1]}")
        for l in model_lines(r["id"]):
            print(l)
        if r["pair"]:
            # pairTech (de TECHS; se llama como la forma que sale en el ataque) con pairPartner (DIGIMON_DATA desde 1):
            # al cambiar a un compañero que tenga esa forma entre sus digievoluciones (fightstg/switch_menu.c)
            t = tech(r["pair"])
            print(f"  técnica doble: «{tname(r['pair'])}» (#{r['pair']}, PM {t['mp']}, poder {t['power'] or '-'}) con "
                  f"{dname(r['pairp'] - 1) if r['pairp'] else '?'}, si el compañero que entra la tiene")
        evo = evolutions()
        if i < 8:
            print(f"  especial (barra): {', '.join(dname(b - 1) for b in r['blast'] if b)}")
            lst = sorted(evo[i][1])
            print(f"  digievoluciones ({len(lst)}):")
            for l in capped([f"    {dname(d)}: {evo_needs(n, c)}" for d, n, c in lst]):
                print(l)
        else:
            src = [(p, n, c) for p, lst in evo for d, n, c in lst if d == i]
            if src:
                print(f"  la consiguen {len(src)} compañeros:")
                for l in capped([f"    {dname(p)}: {evo_needs(n, c)}" for p, n, c in src]):
                    print(l)


def cmd_tecnicas(args):
    """tecnicas [NOMBRE]: datos de una técnica y quién la aprende (sin nombre, la lista)"""
    count = 0x1F26 // 0x12
    if not args:
        lines = [tech_line(n) for n in range(1, count + 1) if tname(n)]
        for l in capped(lines):
            print(l)
        return
    q = " ".join(args).casefold()
    hits = [n for n in range(1, count + 1) if q in tname(n).casefold() or q in tname(n, "E").casefold()]
    lines = []
    for n in hits:
        lines.append(tech_line(n))
        who = []
        for r in digimon_rows():
            if not r["id"]:
                continue
            for k, s in enumerate(r["skills"]):
                if s == n:
                    lv = (f"nv{r['levels'][k - 1]}" if 1 <= k <= 5 else "ataque básico" if k == 0 else
                          "característica" + (f" nv{r['levels'][5]}" if r["levels"][5] else ""))
                    who.append(f"{dname(r['i'])} {lv}")
            if r["pair"] == n:
                who.append(f"{dname(r['i'])} (doble)")
        if who:
            lines.append("    la tienen: " + ", ".join(who))
    print(f"{len(hits)} técnicas")
    for l in capped(lines):
        print(l)


def cmd_evos(args):
    """evos [NOMBRE]: digievoluciones de un compañero, o quién llega a una forma (sin nombre, resumen)"""
    evo = evolutions()
    if not args:
        for p, lst in evo:
            print(f"{dname(p)}: {len(lst)} formas")
        return
    found = find_digimon(" ".join(args))
    if not found:
        sys.exit("ningún Digimon con ese nombre")
    i = found[0]
    if i < 8:
        lines = [f"{dname(d)}: {evo_needs(n, c)}" for d, n, c in sorted(evo[i][1])]
        print(f"{dname(i)}: {len(lines)} digievoluciones")
    else:
        lines = [f"{dname(p)}: {evo_needs(n, c)}" for p, lst in evo for d, n, c in lst if d == i]
        print(f"{dname(i)}: la consiguen {len(lines)} compañeros")
    for l in capped(lines):
        print("  " + l)


# ---------------------------------------------------------------- enemigos (enemigos)

# BattleTableEntry (include/fightstg/battle.h), 0x46 bytes, hasta id 0: id, item, itemChance (1024avos menos uno),
# nameId (DIGNAM), techs[3], stats[5] (ata def esp sab vel), resist[12] (7 elementos, 5 estados), family
ENEMY_FMT = "<4h3h5h12hB"
ENEMY_SIZE = 0x46


@functools.lru_cache(None)
def enemy_table():
    """{luchador: dict} de la tabla de enemigos (archivo 0x1CF, SDIGIEDT.PRO)"""
    b = game_file(0x1CF)
    out = {}
    for k in range(len(b) // ENEMY_SIZE):
        e = struct.unpack_from(ENEMY_FMT, b, k * ENEMY_SIZE)
        if e[0] == 0:
            break
        out[e[0]] = dict(k=k, id=e[0], item=e[1], chance=e[2], name=e[3], techs=e[4:7], stats=e[7:12],
                         res=e[12:24], family=e[24])
    return out


@functools.lru_cache(None)
def encounters():
    """(enemigos: [(luchador, nivel, PV, PM, fuerza)], encuentros: [(i1, i2, i3, kind)]) del FIELDSTG.PRO europeo: el C
    (fieldstg/data/fieldstg.c) trae los enemigos de la versión USA, 538, y el europeo tiene 523 en otro orden"""
    syms = {s[1]: s for s in symbols_of(CFG / "symbols_fieldstg.txt")}
    ovl = game_file("FIELDSTG.PRO")
    vram = 0x80082CB0
    ea, esize = syms["FIELDSTG_enemies"][0], syms["FIELDSTG_enemies"][2]
    enemies = [struct.unpack_from("<i4h", ovl, ea - vram + k * 12) for k in range(esize // 12)]
    ca, csize = syms["FIELDSTG_encounters"][0], syms["FIELDSTG_encounters"][2]
    encs = []
    for k in range(csize // 0x1C):  # Encounter: 3 punteros, ambushChance, kind, blocks[12]
        p = struct.unpack_from("<3I", ovl, ca - vram + k * 0x1C)
        kind = ovl[ca - vram + k * 0x1C + 0xD]
        encs.append(tuple((x - ea) // 12 for x in p) + (kind,))
    return enemies, encs


@functools.lru_cache(None)
def enemy_places():
    """{luchador: [(mapa, zona o 'evento', nivel, PV, [otros luchadores])]} de las batallas de cada mapa (places de su
    JSON: zonas 0-2 al azar, la 3 las de los eventos, FIELDSTG_startEventBattle)"""
    enemies, encs = encounters()
    out = {}
    for st, d in stages_json().items():
        # las batallas de evento que el mapa lanza de verdad (EVENT_BATTLE(n) en su C): muchos traen la lista llena
        # de otras que nadie lanza
        c = next(iter((DECOMP / "src" / "stages").rglob(st + ".c")), None)
        used = {int(x, 0) for x in re.findall(r"EVENT_BATTLE\((0x[0-9A-Fa-f]+|\d+)\)", read(c))} if c else set()
        for p in d.get("places") or []:
            for ai, area in enumerate(p.get("areas", [])):
                for bi, (enc, _, _) in enumerate(area.get("battles", [])):
                    if not enc or enc >= len(encs) or (ai == 3 and bi not in used):
                        continue
                    group = [enemies[i] for i in encs[enc][:3] if i and i < len(enemies) and enemies[i][0]]
                    for k, en in enumerate(group):
                        where = f"evento {bi}" if ai == 3 else f"zona {ai}"
                        others = tuple(sorted(x[0] for j, x in enumerate(group) if j != k))
                        out.setdefault(en[0], []).append((st, where, en[1], en[2], others))
    return out


@functools.lru_cache(None)
def fighters_file():
    """{luchador: (es_enemigo, FighterInfo)} del archivo de luchadores (0x1CC, SMDLDATA.PRO; fightstg/model.h):
    cabecera {base, entries, partners, enemies}, FighterEntry {id, index, kind} hasta id 0 (enemigo si kind >= 0x3A),
    FighterInfo de 196 bytes, el de un enemigo de 0x46 (0x48 alineado)"""
    b = game_file(0x1CC)
    _, ent, par, ene = struct.unpack_from("<4i", b, 0)
    out, k = {}, 0
    while True:
        fid, idx, kind = struct.unpack_from("<hBB", b, ent + 4 * k)
        if fid == 0:
            break
        enemy = kind >= 0x3A
        off = ene + idx * 0x48 if enemy else par + idx * 196
        model, motions, effects, face, distance = struct.unpack_from("<4ih", b, off)
        height = struct.unpack_from("<h", b, off + 0x18)[0]
        cams = struct.unpack_from("<h", b, off + 0x44)[0] if enemy else 12
        out[fid] = dict(enemy=enemy, model=model, motions=motions, effects=effects, distance=distance, height=height,
                        cams=cams, kind=kind)
        k += 1
    return out


def archive_part(ref):
    """(nombre del archivo, su parte, los bytes de la parte) de una referencia archivo << 16 | parte"""
    f, part = ref >> 16, ref & 0xFFFF
    a = game_file(f)
    n = struct.unpack_from("<I", a, 0)[0] // 4
    offs = struct.unpack_from(f"<{n}I", a, 0)
    end = offs[part + 1] if part + 1 < n else len(a)
    names = file_names()
    return (names[f] if f < len(names) else f"{f:#x}"), part, a[offs[part]:end]


def used_slots(table):
    """los índices con algo de una tabla de posiciones (la primera palabra, su tamaño; 0, vacío)"""
    if len(table) < 4:
        return []
    n = min(struct.unpack_from("<I", table, 0)[0] // 4, len(table) // 4)
    return [i for i, v in enumerate(struct.unpack_from(f"<{n}I", table, 0)) if v]


def ranges(ns):
    out, start = [], None
    for i, n in enumerate(ns):
        if start is None:
            start = n
        if i + 1 == len(ns) or ns[i + 1] != n + 1:
            out.append(f"{start}" if start == n else f"{start}-{n}")
            start = None
    return ",".join(out)


def model_lines(fighter):
    """el modelo, los movimientos y los guiones de efectos de un luchador, en dos líneas"""
    info = fighters_file().get(fighter)
    if not info:
        return [f"  sin entrada en SMDLDATA.PRO (luchador {fighter})"]
    try:
        mfile, mpart, mdata = archive_part(info["model"])
        _, apart, motions = archive_part(info["motions"])
        _, epart, effects = archive_part(info["effects"])
    except (ValueError, struct.error, IndexError, FileNotFoundError):
        return [f"  modelo {info['model']:#x}, movimientos {info['motions']:#x}, efectos {info['effects']:#x}"]
    mv, ef = used_slots(motions), used_slots(effects)
    whole = game_file(info["model"] >> 16)
    parts = struct.unpack_from("<I", whole, 0)[0] // 4
    height = f"altura {info['height']}, " if info["height"] else ""
    return [f"  modelo: {mfile} ({parts} partes, {len(whole) // 1024} KB), el modelo en la parte {mpart}; {height}"
            f"distancia {info['distance']}, {info['cams']} cámaras",
            f"  movimientos ({len(mv)}, parte {apart}): {ranges(mv)}; guiones de efectos ({len(ef)}, parte {epart}): "
            f"{ranges(ef) or '-'}"]


def enemy_name(fighter, lang="S"):
    e = enemy_table().get(fighter)
    ns = names_of("DIGNAM", lang)
    return ns[e["name"]] if e and e["name"] < len(ns) else f"luchador {fighter}"


def item_name(i):
    ns = names_of("ITMNAM", "S")
    return ns[i] if 0 <= i < len(ns) else f"objeto {i:#x}"


def cmd_enemigos(args):
    """enemigos [NOMBRE | luchador]: técnicas, stats, resistencias, lo que suelta y en qué mapas sale (sin nombre, la
    lista)"""
    table = enemy_table()
    if not args:
        lines = [f"{f} {enemy_name(f)}" for f in sorted(table, key=lambda f: enemy_name(f))]
        print(f"{len(table)} enemigos (luchador nombre):")
        for l in capped(lines, MAX * 5 if MAX else 0):
            print("  " + l)
        return
    q = " ".join(args)
    if q.isdigit() and int(q) in table:
        found = [int(q)]
    else:
        qc = q.casefold()
        found = [f for f in table if qc == enemy_name(f).casefold() or qc == enemy_name(f, "E").casefold()]
        found = found or [f for f in table if qc in enemy_name(f).casefold() or qc in enemy_name(f, "E").casefold()]
    if not found:
        sys.exit("ningún enemigo con ese nombre (dw3.py enemigos los lista)")
    places = enemy_places()
    for f in found[:MAX or None]:
        e = table[f]
        print(f"{both(enemy_name(f), enemy_name(f, 'E'))} (luchador {f}, entrada {e['k']} de SDIGIEDT.PRO, familia {e['family']})")
        techs = [tname(t) if tname(t) not in ("", "a") else f"#{t}" for t in e["techs"] if t]
        print(f"  técnicas: {', '.join(techs)}  stats (ata def esp sab vel) {list(e['stats'])}")
        print(f"  resist. (fue agu hie vie ray máq osc) {list(e['res'][:7])}  (vene par conf sue KO) {list(e['res'][7:])}")
        if e["item"]:
            print(f"  suelta: {item_name(e['item'])} ({(e['chance'] + 1) * 100 / 1024:.1f}%)")
        for l in model_lines(f):
            print(l)
        seen = {}
        for st, where, lv, hp, others in places.get(f, []):
            seen.setdefault((st, where, lv, hp, others), 0)
            seen[(st, where, lv, hp, others)] += 1
        lines = []
        for (st, where, lv, hp, others), n in seen.items():
            name = stage_places().get(st, ("?", ""))[0].split(",")[0]
            con = f" con {', '.join(enemy_name(o) for o in others)}" if others else ""
            lines.append(f"    {st} [{stage_mode(st)}] {name}: {where} nv{lv} PV{hp}{con}" + (f" (×{n})" if n > 1 else ""))
        print(f"  aparece en {len(lines)} batallas:" if lines else "  no aparece en las batallas de los mapas")
        for l in capped(lines):
            print(l)


# ---------------------------------------------------------------- salidas de los mapas (salidas)

# StageSlot (include/field_map.h), como lo traen los JSON de los mapas: conditions[2][2], type, arg, x, y, dir,
# hideAnim, place, placeArg. Las que llevan a otro modo: arg es el modo (en los warps y lanzamientos, su SlotDest)
EXIT_KINDS = {1: "puerta", 9: "warp1", 10: "warp0", 14: "lanzamiento"}


@functools.lru_cache(None)
def stage_places():
    """{wstagNNN: (nombre, zona)} de dw3_decomp/src/stages/README.md"""
    out, zone = {}, ""
    for l in read(DECOMP / "src" / "stages" / "README.md").split("\n"):
        m = re.match(r"## (.*?) \(`", l)
        if m:
            zone = m.group(1)
        m = re.match(r"- `(wstag\d+)`: (.*)", l)
        if m:
            out[m.group(1)] = (m.group(2), zone)
    return out


def place_points(stage):
    """las salas de un mapa que rellena sus salidas al entrar (copyPlacePoints, stages/common): {(place, placeArg):
    [(modo, x, y)]}, el punto i va a la salida i de stageSlots; {} si no las tiene"""
    c = next(iter((DECOMP / "src" / "stages").rglob(stage + ".c")), None)
    if not c or "copy_place_points" not in read(c):
        return {}
    t = read(c)
    v = r"(0x[0-9A-Fa-f]+|\d+)"
    points = {m.group(1): (num(m.group(2)), num(m.group(5)), num(m.group(6)), m.group(8))
              for m in re.finditer(r"StagePoint (\w+) = \{ %s, %s, %s, %s, %s, %s, &?(\w+) \};" % ((v,) * 6), t)}
    out = {}
    for m in re.finditer(r"StagePoints (\w+) = \{ %s, %s, &(\w+) \};" % (v, v), t):
        lst, k = [], m.group(4)
        while k in points:
            mode, x, y, k = points[k]
            lst.append((mode, x, y))
        out[(num(m.group(2)), num(m.group(3)))] = lst
    return out


@functools.lru_cache(None)
def exits_graph(extra=False):
    """{modo: (wstag, [(tipo, destino, x, y, condiciones, sala)])}; sala: (place, placeArg) en los mapas que eligen sus
    salidas al entrar, o None. Sin el capítulo extra (wstag92x-97x) salvo extra"""
    import quest_talks as q
    out = {}
    for st, d in stages_json().items():
        if (st >= "wstag920") != extra and not (extra and st < "wstag920"):
            continue
        modes = d.get("euModes") or d.get("modes") or []
        if not modes:
            continue
        lst = []
        slots = d.get("slots", [])
        rooms = place_points(st)
        for k, s in enumerate(slots):
            if s[4] not in EXIT_KINDS:
                continue
            conds = [c for c in (q.code(s[i], s[i + 1]) for i in (0, 2) if s[i] != 0xFFFF) if c]
            if rooms:
                for room, pts in rooms.items():
                    if k < len(pts):
                        lst.append((EXIT_KINDS[s[4]], pts[k][0], pts[k][1], pts[k][2], conds, room))
            else:
                lst.append((EXIT_KINDS[s[4]], s[5], s[6], s[7], conds, None))
        mode = modes[0][0]
        if mode in out and not extra:
            out[mode][1].extend(lst)
        else:
            out[mode] = (st, lst)
    return out


def stage_mode(stage):
    """el modo de un mapa (euModes[0][0] de su JSON), en hex; '?' si no tiene"""
    d = stages_json().get(stage) or {}
    modes = d.get("euModes") or d.get("modes") or []
    return f"{modes[0][0]:#x}" if modes else "?"


def mode_label(g, m):
    if m not in g:
        return f"{m:#x}"
    st = g[m][0]
    name, zone = stage_places().get(st, ("?", "?"))
    return f"{m:#x} {st} ({name.split(',')[0]})"


def cmd_salidas(args):
    """salidas [MODO_hex] [--extra]: puertas, warps y lanzamientos de un modo, de dónde se llega y a qué zonas sale
    andando (sin modo: las salas sin salida y las zonas)"""
    extra = "--extra" in args
    args = [a for a in args if a != "--extra"]
    g = exits_graph(extra)
    zone = {m: stage_places().get(st, ("", "?"))[1] for m, (st, _) in g.items()}
    if not args:
        none = sorted(m for m, (_, ex) in g.items() if not [e for e in ex if e[1] != m])
        print(f"{len(g)} modos; sin salida a otro modo ({len(none)}): " + ", ".join(mode_label(g, m) for m in none))
        by_zone = {}
        for m in g:
            by_zone.setdefault(zone[m], []).append(m)
        for z, ms in sorted(by_zone.items()):
            print(f"  {z}: {len(ms)} modos ({min(ms):#x}-{max(ms):#x})")
        return
    a = args[0].lower().removesuffix(".json").removesuffix(".c")
    if a.startswith("wstag"):  # el archivo del mapa en lugar de su modo
        st = [k for k, (s, _) in g.items() if s == a]
        if not st:
            sys.exit(f"{a}: no está entre los mapas{' del capítulo extra' if extra else ''} (¿--extra?)")
        m = st[0]
    else:
        try:
            m = int(a, 16)
        except ValueError:
            sys.exit("uso: dw3.py salidas [0x230 | wstag430] [--extra]")
    if m not in g:
        sys.exit(f"{m:#x}: ningún mapa{' del capítulo extra' if extra else ''} con ese modo")
    st, ex = g[m]
    name, z = stage_places().get(st, ("?", "?"))
    print(f"{m:#x} {st}: {name} [{z}]")
    lines = []
    if any(e[5] for e in ex):
        print("  sus salidas dependen de la sala, (GAME.place, GAME.placeArg), con la que se entra (copyPlacePoints)")
    grouped = {}  # la misma salida en varias salas, una línea
    for kind, dest, x, y, conds, room in ex:
        grouped.setdefault((kind, dest, x, y, tuple(conds)), []).append(room)
    for (kind, dest, x, y, conds), rooms in grouped.items():
        rs = [r for r in rooms if r]
        sala = ("salas " if len(rs) > 1 else "sala ") + " ".join(f"{a},{b}" for a, b in rs) + ": " if rs else ""
        lines.append(f"  {sala}{kind} → {mode_label(g, dest)} en ({x}, {y})" + (f" si {', '.join(conds)}" if conds else ""))
    print(f"salidas ({len(ex)}):" if ex else "salidas: ninguna")
    for l in capped(lines, MAX * 2 if MAX else 0):
        print(l)
    came = sorted({src for src, (_, e) in g.items() for k, d, *_ in e if d == m and src != m})
    print("se llega desde: " + (", ".join(mode_label(g, s) for s in came) if came else "ningún modo (evento, menú o viaje)"))
    # andando (sin mirar condiciones): el camino más corto a cada zona distinta
    prev, todo, reached = {m: None}, [m], {}
    while todo:
        cur = todo.pop(0)
        for _, d, *_ in g.get(cur, ("", []))[1]:
            if d in prev:
                continue
            prev[d] = cur
            if zone.get(d, "?") != z:
                reached.setdefault(zone.get(d, "?"), d)
            else:
                todo.append(d)
    if not reached:
        print(f"andando no sale de {z} (sin mirar condiciones)")
    for oz, d in sorted(reached.items()):
        path, k = [], d
        while k is not None:
            path.append(f"{k:#x}")
            k = prev[k]
        print(f"  a {oz}: {' → '.join(reversed(path))}")


# ---------------------------------------------------------------- los scripts de historia y misiones

def run_script(name, argv):
    """ejecuta tools/<name>.py con esos argumentos y recorta su salida a --max líneas"""
    out = io.StringIO()
    old = sys.argv
    sys.argv = [name + ".py"] + argv
    try:
        with contextlib.redirect_stdout(out):
            runpy.run_path(str(TOOLS / f"{name}.py"), run_name="__main__")
    except SystemExit:
        pass
    finally:
        sys.argv = old
    lines = out.getvalue().rstrip("\n").split("\n")
    # sin las cabeceras de mapa o paso que se quedan vacías (quest_talks --busca las saca todas)
    head = re.compile(r"^(====|########) ")
    lines = [l for k, l in enumerate(lines) if not head.match(l) or (k + 1 < len(lines) and not head.match(lines[k + 1]))]
    # cada mapa con su modo (el número del wstag no lo es: wstag225 es el modo 0x207), salvo donde ya sale
    lines = [l if "modos" in l or "['0x" in l else re.sub(r"\b(wstag\d+)\b(?! \[0x)", lambda m: f"{m.group(1)} [{stage_mode(m.group(1))}]", l)
             for l in lines]
    for l in capped(lines, MAX * 3 if MAX else 0, "salen --max×3 líneas; --max 0 para todas"):
        print(l)


def script_cmd(name, usage):
    def f(args):
        if name == "quest_talks":  # sus modos también como el archivo del mapa: wstag875 → su modo
            args = [stage_mode(a.lower()) if a.lower().startswith("wstag") else a for a in args]
            if "?" in args:
                sys.exit("ese wstag no es un mapa del juego extraído")
        run_script(name, args)
    f.__doc__ = usage
    return f


def cmd_digilog(args):
    """digilog [paso_hex | objeto_hex]: los pasos del Digilog (journal_steps.py) de un paso de historia o misión"""
    import journal_steps as j

    def code(c):
        if c == j.ALL:
            return "TODOS"
        if c >= 0xF000:
            return f"tras paso {c & 0xFFF:#x}"
        if 0x9200 <= c < 0x9400:
            return f"carta {c & 0x1FF:#x}"
        if 0x8000 <= c < 0x9000:
            return f"objeto {c & 0x1FF:#x}"
        if 0x7200 <= c < 0x7400:
            import quest_talks as q
            return q.party_stat(c & 0x1FF, 1)
        return flag_text(c)

    def where(w):
        return "" if not w else (f"modo {w[1]:#x}" if w[0] == "m" else f"wstag{w[1]}")

    want = {int(a, 16) for a in args}
    lines = []
    for steps_of, steps in j.STORY:
        if want and not want & set(steps_of):
            continue
        lines.append(f"historia {', '.join(hex(p) for p in steps_of)}:")
        lines += [f"  {code(c)} {where(w)}: {es}" for c, w, es, _ in steps]
    for item in sorted(j.QUEST_STEPS):
        if want and item not in want:
            continue
        lines.append(f"misión (objeto {item:#x}):")
        lines += [f"  {code(c)} {where(w)}: {es}" for c, w, es, _ in j.QUEST_STEPS[item]]
    for l in capped(lines, MAX * 3 if MAX else 0):
        print(l)


COMMANDS = {
    "addr": cmd_addr,
    "sym": cmd_sym,
    "texto": cmd_texto,
    "marca": cmd_marca,
    "digimon": cmd_digimon,
    "tecnicas": cmd_tecnicas,
    "evos": cmd_evos,
    "digilog": cmd_digilog,
    "salidas": cmd_salidas,
    "enemigos": cmd_enemigos,
    "pasos": script_cmd("story_steps", "pasos [paso_hex ...]: conversaciones que hacen algo en cada paso de historia (story_steps.py)"),
    "eventos": script_cmd("story_events", "eventos [paso_hex ...]: cadena de eventos de cada paso de historia (story_events.py)"),
    "codigo": script_cmd("story_code", "codigo [paso_hex ...]: dónde cambia GAME.progress y qué marcas activa el código (story_code.py)"),
    "dialogo": script_cmd("story_dialogue", "dialogo EVENTO|wstagNNN:EVENTO ...: frases de un evento, en español (story_dialogue.py)"),
    "charlas": script_cmd("quest_talks", "charlas MODO_hex ... | --busca CÓDIGO[,..]: conversaciones con condiciones y acciones (quest_talks.py)"),
    "candidatas": script_cmd("quest_candidates", "candidatas: conversaciones que dan objetos, misiones candidatas (quest_candidates.py)"),
}


def main():
    global MAX
    args = sys.argv[1:]
    if "--max" in args:
        i = args.index("--max")
        MAX = int(args[i + 1])
        del args[i:i + 2]
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    if not args or args[0] in ("-h", "--help", "help") or args[0] not in COMMANDS:
        if args and args[0] not in ("-h", "--help", "help"):
            print(f"subcomando desconocido: {args[0]}")
        print("dw3.py SUBCOMANDO ... [--max N]")
        for name, f in COMMANDS.items():
            print("  " + (f.__doc__ or name).strip().split("\n")[0])
        return
    if "--max" not in sys.argv and args[0] == "texto":
        MAX = 8  # cada texto saca hasta 6 líneas
    COMMANDS[args[0]](args[1:])


if __name__ == "__main__":
    main()
