#!/usr/bin/env python3
"""Exports the battle models (the fighters' and the effects') and their motions for a local 3D viewer.

    python tools/modelos.py [--out DIR] [--only AGUM ...] [--check]

Reads the extracted game (native/build/data or test2/data, as dw3.py) and writes to native/build/modelos/
(ignored by git: the models are the game's, they never go to the repo):
  list.js        every model: its key, name, group, file, bones and motions
  m/<key>.js     one model: bones, meshes, keys, motions, face and the VRAM its textures fill
  index.html     the viewer (a copy of tools/modelos_viewer.html): open it with a double click
--check reads every model and prints what looks wrong (nothing is written). --only lists only those models.
The page takes three.js 0.160.0 from cdn.jsdelivr.net (an import map), so it needs the internet.

The format (dw3_decomp src/fightstg/fightstg.c, face.c, effect_model.c; src/main/gfx/tim_loader.c):
- The fighters' file (0x1CC, SMDLDATA.PRO; dw3.fighters_file) gives each fighter's model (file << 16 | part, the
  bone list), its motions (file << 16 | part) and its face (an offset in that file, from its base).
  FIGHTSTG_effectModels (FIGHTSTG.PRO) gives each effect model's {id, file, motionFile}.
- A file is an archive (s32 offsets from its start); a part may be an archive too (a mesh, a bone's keys).
- Bone list: s32 texPart, n, then n x (parent, meshPart, keysPart); the parts are in the bone list's file. Bone 0
  is the root (no mesh); entry k is bone k + 1.
- Mesh: entry 0 vertices and 1 normals (s16 x,y,z each; the first one's x is the count), 2 commands, 5 bounds.
  Commands, byte by byte until 0xFF: a high nibble 8..E sets quad, textured, -, -, lit, gouraud, abr (semi-trans:
  abr - 1 is the PSX blend mode) to the low nibble; 0x01 + 6 bytes sets the texture (u offset b1, page x b2 * 64,
  v offset b3, CLUT (b4, b5), colour depth b6); 0x02..0x05 + 3 bytes a flat colour; 0x00 starts polygons: each is a
  0 byte, then 3 or 4 vertex indices, 3 or 4 normal indices if lit, (u, v) pairs if textured; while the next byte
  is 0. A polygon whose screen winding is clockwise is drawn (nclip > 0).
- Keys (an archive of 3: positions, rotations, scales): s16 x, y, z, frame each, until frame 30000; values between
  two keys are linear. Rotation 4096 = 360 degrees, scale 4096 = 1; a bone whose scales OR'ed are <= 48 is hidden
  (its world matrix is not updated). local = Rz * Ry * Rx * Scale, then the position; world = parent * local.
- Motions (an archive, motion N is entry N - 1): s16 {index, count, frame, endFrame} steps until index 0x7FFF;
  FIGHTSTG_setMotion / FIGHTSTG_stepMotion, as the viewer replays them, at 60 keyframes a second.
- Textures (texPart, an archive of TIMs ending in a 0 offset): 16-bit TIMs, RLEN ("RLEN" + 4 bytes, then runs:
  c & 0x80 repeats the next byte c & 0x7F times, else copies c bytes; 0 ends), each placed 64 halfwords right of
  the one before. The 4 and 8-bit pages and their CLUTs are inside those images: the viewer reads them as VRAM.
- Face: up to 16 FaceRect {x, y, w, h, frames[3][2]} until x 0xFF (w 0: unused): parts 0-1 (eyes) blink, the
  others cycle 3 frames, each a VRAM copy of frames[f] over (x, y).
"""
import argparse
import base64
import json
import shutil
import struct
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
import dw3  # noqa: E402

OUT = TOOLS.parent / "build" / "modelos"
VIEWER = TOOLS / "modelos_viewer.html"
FIGHTERS_FILE = 0x1CC
FIGHTSTG_VRAM = 0x80082CB0
KEY_END = 30000


def table(buf, off=0):
    """the offsets of the archive at off (the first one is the table's size)"""
    n = struct.unpack_from("<I", buf, off)[0] // 4
    return list(struct.unpack_from(f"<{n}I", buf, off))


def entry(buf, arch, i):
    return arch + struct.unpack_from("<I", buf, arch + 4 * i)[0]


def rlen(buf, off):
    out, src = bytearray(), off + 8
    while buf[src]:
        c = buf[src]
        if c & 0x80:
            out += bytes([buf[src + 1]]) * (c & 0x7F)
            src += 2
        else:
            out += buf[src + 1:src + 1 + c]
            src += 1 + c
    return bytes(out)


def textures(buf, arch, warn):
    """the VRAM the texture archive fills: (width, height, bytes of u16)"""
    images, x = [], 0
    for i in range(64):
        o = struct.unpack_from("<I", buf, arch + 4 * i)[0]
        if o == 0:
            break
        a = arch + o
        tim = rlen(buf, a) if buf[a:a + 4] == b"RLEN" else buf[a:]
        flag = struct.unpack_from("<I", tim, 4)[0]
        q = 8
        if flag & 8:
            warn(f"TIM {i} with its own CLUT (mode {flag & 7}): not placed")
            q += struct.unpack_from("<I", tim, q)[0]
        _, _, _, w, h = struct.unpack_from("<I4H", tim, q)
        images.append((x, w, h, tim[q + 12:q + 12 + w * h * 2]))
        x += 64
    if not images:
        return 0, 0, b""
    width = max(64 * len(images), max(ix + w for ix, w, _, _ in images))
    height = max(h for _, _, h, _ in images)
    vram = bytearray(width * height * 2)
    for ix, w, h, px in images:
        for row in range(h):
            d = (row * width + ix) * 2
            vram[d:d + w * 2] = px[row * w * 2:(row + 1) * w * 2]
    return width, height, bytes(vram)


def vectors(buf, off):
    n = struct.unpack_from("<h", buf, off)[0]
    return n, list(struct.unpack_from(f"<{3 * n}h", buf, off + 6))


def mesh(buf, arch, warn):
    """{v: vertices, n: normals, mats: [[pageX, depth, clutX, clutY]], p: polygons} of a mesh; a polygon is
    [flags, mat, colour, idx..., nrm..., uv...] with flags quad 1, textured 2, lit 4, gouraud 8, abr << 4"""
    nv, verts = vectors(buf, entry(buf, arch, 0))
    nn, norms = vectors(buf, entry(buf, arch, 1))
    c = entry(buf, arch, 2)
    st = dict(quad=0, tex=0, lit=0, gou=0, abr=0)
    mats, mat, colour, polys = [], -1, 0x808080, []
    while buf[c] != 0xFF:
        op = buf[c]
        hi, lo = op >> 4, op & 0xF
        if hi:
            key = {8: "quad", 9: "tex", 12: "lit", 13: "gou", 14: "abr"}.get(hi)
            if key:
                st[key] = lo
            c += 1
        elif lo == 1:
            b = buf[c + 1:c + 7]
            m = [b[1] * 64, b[5], b[3], b[4], b[0], b[2]]  # page x, depth, clut x, y, u and v offsets
            if m not in mats:
                mats.append(m)
            mat = mats.index(m)
            c += 7
        elif 2 <= lo <= 5:
            if lo == 2:
                colour = buf[c + 1] << 16 | buf[c + 2] << 8 | buf[c + 3]
            c += 4
        elif lo == 0:
            while True:
                c += 1
                n = 4 if st["quad"] else 3
                idx = list(buf[c:c + n])
                p = c + n
                nrm = list(buf[p:p + n]) if st["lit"] else []
                p += n if st["lit"] else 0
                uv = list(buf[p:p + 2 * n]) if st["tex"] else []
                if max(idx) >= nv or (nrm and max(nrm) >= nn):
                    warn(f"polygon past the mesh's {nv} vertices / {nn} normals")
                flags = (n == 4) | (bool(st["tex"]) << 1) | (bool(st["lit"]) << 2) | (bool(st["gou"]) << 3) | (st["abr"] << 4)
                polys.append([flags, mat if st["tex"] else -1, colour] + idx + nrm + uv)
                c += n + (n if st["lit"] else 0) + (2 * n if st["tex"] else 0)
                if buf[c] != 0:
                    break
        else:
            warn(f"unknown mesh command {op:#x}")
            break
    return dict(v=verts, n=norms, mats=mats, p=polys)


def keys(buf, arch):
    out = []
    for i in range(3):
        o, k = entry(buf, arch, i), []
        while True:
            x, y, z, f = struct.unpack_from("<4h", buf, o)
            k += [x, y, z, f]
            o += 8
            if f >= KEY_END:
                break
        out.append(k)
    return out


def motions(ref):
    buf = dw3.game_file(ref >> 16)
    arch = table(buf)[ref & 0xFFFF]
    out = []
    for o in table(buf, arch):
        steps, a = [], arch + o
        if o == 0:
            out.append([])
            continue
        while True:
            s = list(struct.unpack_from("<4h", buf, a))
            a += 8
            if s[0] == 0x7FFF:
                break
            steps.append(s)
        out.append(steps)
    return out


def face(fighter):
    b = dw3.game_file(FIGHTERS_FILE)
    base, ent, par, ene = struct.unpack_from("<4i", b, 0)
    k = 0
    while True:
        fid, idx, kind = struct.unpack_from("<hBB", b, ent + 4 * k)
        if fid == 0:
            return []
        if fid == fighter:
            break
        k += 1
    off = (ene + idx * 0x48) if kind >= 0x3A else (par + idx * 196)
    f = struct.unpack_from("<i", b, off + 12)[0] - base
    rects = []
    for i in range(16):
        r = b[f + 10 * i:f + 10 * i + 10]
        if r[0] == 0xFF:
            break
        rects.append(list(r))
    return rects


def model(ref, motion_ref, warn):
    f, part = ref >> 16, ref & 0xFFFF
    buf = dw3.game_file(f)
    parts = table(buf)
    bl = parts[part]
    tex, n = struct.unpack_from("<2i", buf, bl)
    bones, meshes, keyset = [], [], []
    for k in range(n):
        parent, mp, kp = struct.unpack_from("<3i", buf, bl + 8 + 12 * k)
        bones.append(parent)
        meshes.append(mesh(buf, parts[mp & 0xFFFF], warn))
        keyset.append(keys(buf, parts[kp & 0xFFFF]))
    w, h, vram = textures(buf, parts[tex & 0xFFFF], warn) if tex else (0, 0, b"")
    for m in meshes:
        for px, depth, cx, cy, _, _ in m["mats"]:
            if cx + (16 if depth == 0 else 256) > w or cy >= h or px >= w:
                warn(f"texture page {px} / CLUT ({cx}, {cy}) outside the model's VRAM {w}x{h}")
    return dict(parents=bones, meshes=meshes, keys=keyset, motions=motions(motion_ref), vram=[w, h, base64.b64encode(vram).decode()])


def effect_models():
    """FIGHTSTG_effectModels: [(id, file ref, motions ref)]"""
    syms = {s[1]: s for s in dw3.symbols_of(dw3.CFG / "symbols_fightstg.txt")}
    ovl = dw3.game_file("FIGHTSTG.PRO")
    o = syms["FIGHTSTG_effectModels"][0] - FIGHTSTG_VRAM
    out = []
    while True:
        i, f, m = struct.unpack_from("<3i", ovl, o)
        if i == 0:
            return out
        out.append((i, f, m))
        o += 12


def catalogue():
    """[(key, name, group, model ref, motions ref, fighter or None)]"""
    ff = dw3.fighters_file()
    names = dw3.file_names()
    out = []
    partners = {r["id"]: r["i"] for r in dw3.digimon_rows()}
    for fid in sorted(ff, key=lambda x: (ff[x]["enemy"], partners.get(x, 999), x)):
        info = ff[fid]
        if fid in partners:
            i = partners[fid]
            name, group = dw3.dname(i), ("Compañeros" if i < 8 else "Formas")
        else:
            name, group = dw3.enemy_name(fid), "Enemigos"
        out.append((f"f{fid}", name, group, info["model"], info["motions"], fid))
    seen = set()
    for i, f, m in effect_models():
        if i in seen:  # FIGHTSTG_startEffectModel takes the first entry of an id
            continue
        seen.add(i)
        out.append((f"e{i}", f"Efecto {i}", "Efectos", f, m, None))
    return out, names


def export(out, only=None, check=False):
    cat, names = catalogue()
    if only:
        cat = [c for c in cat if any(o.lower() in (c[1] + names[c[3] >> 16] + c[0]).lower() for o in only)]
    if not check:
        (out / "m").mkdir(parents=True, exist_ok=True)
    listing, problems = [], 0
    for key, name, group, ref, mref, fid in cat:
        msgs = []
        try:
            m = model(ref, mref, msgs.append)
        except (struct.error, IndexError) as e:
            msgs.append(f"unreadable: {e}")
            m = None
        fname = names[ref >> 16]
        if msgs:
            problems += 1
            print(f"{key} {name} ({fname}): " + "; ".join(sorted(set(msgs))))
        if m is None:
            continue
        m["face"] = face(fid) if fid else []
        m.update(key=key, name=name, file=fname)
        polys = sum(len(x["p"]) for x in m["meshes"])
        listing.append(dict(key=key, name=name, group=group, file=fname, fighter=fid, bones=len(m["parents"]) + 1,
                            polys=polys, motions=sum(1 for s in m["motions"] if s)))
        if not check:
            (out / "m" / f"{key}.js").write_text("modelLoaded(" + json.dumps(m, separators=(",", ":")) + ");\n",
                                                 encoding="utf-8")
    if check:
        print(f"{len(listing)} modelos leídos, {problems} con avisos")
        return
    (out / "list.js").write_text("window.MODELS = " + json.dumps(listing, ensure_ascii=False, indent=0) + ";\n",
                                 encoding="utf-8")
    shutil.copyfile(VIEWER, out / "index.html")
    print(f"{len(listing)} modelos ({problems} con avisos) -> {out / 'index.html'}")


def main():
    ap = argparse.ArgumentParser(description="exporta los modelos 3D de combate y sus movimientos para el visor")
    ap.add_argument("--out", type=Path, default=OUT, help=f"carpeta de salida (por defecto {OUT})")
    ap.add_argument("--only", nargs="*", help="solo los que contengan esto (nombre, archivo o clave: AGUM, f3...)")
    ap.add_argument("--check", action="store_true", help="solo leer todos y avisar de lo raro")
    a = ap.parse_args()
    export(a.out, a.only, a.check)


if __name__ == "__main__":
    main()
