#!/usr/bin/env python3
"""Exports the field characters' sprites (P###XXXX.BIN) and their animations for a local viewer.

    python tools/sprites.py [--out DIR] [--gif] [--only P006AGUM ...]

Reads the extracted game (native/build/data or test2/data, as dw3.py) and writes to native/build/sprites/
(ignored by git: the images are the game's, they never go to the repo):
  sheets/<FILE>.png   every frame of a sprite file, packed in a sheet
  data.js             the frames' places and offsets, the animation sets, the characters that use each file
  index.html          the viewer (a copy of tools/sprites_viewer.html): open it with a double click
  gif/<FILE>/...      with --gif, one GIF per animation set and direction

The format (FIELDSTG_animateActor, FIELDSTG_drawActor in dw3_decomp/src/fieldstg/actor_control.c):
- FIELDSTG_fileEntries[character] = file << 16 | E. The file is an archive (s32 offsets, ending in 0).
- Entry E is the set table: records of 6 s16 [set, d0..d4] until set 0; d is the entry with the frame list
  of direction 0..4 (down, down-left, left, up-left, up); 5..7 mirror 3..1. A missing set plays the first one.
- A frame list is s32 quads (time, TIM entry, x offset, y offset), ending in 0 (loops) or -1 (holds the last
  frame). The time is in 60 Hz vsyncs (getFrameTime), in PAL too.
- Each frame is a TIM, 4 bpp with its own 16 colour CLUT. Its top left is drawn at (x + xOff, y + yOff) from
  the actor's tile; mirrored, at x - (w + xOff).
- Character 2 (the player) takes its sets from other files, by set (SET_FILE_* in actor_control.c).
"""
import argparse
import json
import shutil
import struct
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
import dw3  # noqa: E402

try:
    from PIL import Image
except ImportError:
    sys.exit("hace falta Pillow: python -m pip install pillow")

OUT = TOOLS.parent / "build" / "sprites"
VIEWER = TOOLS / "sprites_viewer.html"

# The animation sets with a name (ACTOR_ANIM_* in include/fieldstg.h)
SET_NAMES = {
    0x01: "quieto", 0x04: "andar", 0x05: "correr", 0x06: "parar", 0x08: "buscar",
    0x11: "medidor: empieza", 0x12: "medidor: resultado", 0x13: "medidor: juega", 0x14: "medidor: espera",
    0x15: "medidor: acaba", 0x16: "bajar", 0x17: "caer", 0x18: "caer lejos", 0x19: "aterrizar",
    0x1A: "trepar: sube", 0x1B: "trepar: baja", 0x1C: "subirse a la pared", 0x1D: "trepar: sale arriba",
    0x1E: "pasar el borde", 0x1F: "trepar: sale abajo", 0x20: "trepar", 0x41: "abrir (tesoro)", 0x45: "usar",
    0x54: "usado (objeto)",
}

# Character 2 (the player): the file (file << 16 | entry) of each set, European (SET_FILE_* in actor_control.c)
PLAYER = 2
PLAYER_FILES = {"0": 0x01870070, "8": 0x03CB001E, "16": 0x03C9000B, "1A": 0x03CC0017, "other": 0x03CA0064}


def player_file(s):
    if s < 8:
        return PLAYER_FILES["0"]
    if s == 8:
        return PLAYER_FILES["8"]
    if s < 0x16:
        return PLAYER_FILES["other"]
    if s < 0x1A:
        return PLAYER_FILES["16"]
    if s < 0x25:
        return PLAYER_FILES["1A"]
    return PLAYER_FILES["other"]


class SpriteFile:
    """One P###XXXX.BIN: its set table at entry `table`, its frame lists and TIM frames"""

    def __init__(self, index, table):
        self.index = index
        self.name = dw3.file_names()[index]
        self.data = dw3.game_file(index)
        self.table = table
        self.tims = {}  # TIM entry -> (w, h, RGBA bytes)

    def entry(self, n):
        return struct.unpack_from("<i", self.data, 4 * n)[0]

    def sets(self):
        """[(set, [entries of directions 0..4])]"""
        out = []
        o = self.entry(self.table)
        while True:
            rec = struct.unpack_from("<6h", self.data, o)
            if rec[0] == 0:
                return out
            out.append((rec[0], list(rec[1:])))
            o += 12

    def frames(self, e):
        """a direction's list: ([(time, TIM entry, xOff, yOff)], loops)"""
        out = []
        o = self.entry(e)
        while True:
            t = struct.unpack_from("<i", self.data, o)[0]
            if t in (0, -1):
                return out, t == 0
            out.append(struct.unpack_from("<4i", self.data, o))
            o += 16

    def tim(self, e):
        """a frame's (w, h, RGBA): 4 bpp; a CLUT colour of 0 is see-through, as on the PlayStation"""
        if e in self.tims:
            return self.tims[e]
        d = self.data
        o = self.entry(e)
        _, flag = struct.unpack_from("<II", d, o)
        if flag & 7 != 0:
            raise ValueError(f"{self.name}: entry {e} is not a 4 bpp TIM (flag {flag:#x})")
        p = o + 8
        clut = [0] * 16
        if flag & 8:
            size, _, _, cw, ch = struct.unpack_from("<IHHHH", d, p)
            clut = list(struct.unpack_from("<16H", d, p + 12))
            p += size
        _, _, _, w, h = struct.unpack_from("<IHHHH", d, p)
        w *= 4
        pal = []
        for c in clut:
            r, g, b = (c & 31) << 3, (c >> 5 & 31) << 3, (c >> 10 & 31) << 3
            pal.append(bytes((r | r >> 5, g | g >> 5, b | b >> 5, 0 if c == 0 else 255)))
        px = d[p + 12:p + 12 + w * h // 2]
        out = bytearray()
        for byte in px:
            out += pal[byte & 15] + pal[byte >> 4]
        self.tims[e] = (w, h, bytes(out))
        return self.tims[e]


def pack(sizes, width=512):
    """shelf packing: [(x, y)] for each (w, h), and the sheet's size"""
    x = y = shelf = 0
    places = []
    for w, h in sizes:
        if x + w > width:
            x, y, shelf = 0, y + shelf, 0
        places.append((x, y))
        x += w
        shelf = max(shelf, h)
    return places, (width if y else max(x, 1), y + shelf)


def character_uses():
    """{character: [map names]} of the maps' actors"""
    places = dw3.stage_places()
    out = {}
    for n, uses in dw3.character_uses().items():
        names = []
        for st, _, _, _ in uses:
            name = f"{st} {places.get(st, ('?', ''))[0].split(',')[0]}"
            if name not in names:
                names.append(name)
        out[n] = names
    return out


def export(out, gif=False, only=None):
    sprites = dw3.character_sprites()
    uses = character_uses()
    files = {}  # file index -> SpriteFile
    chars = {}  # entry name -> {"keys", "names", "maps", "sets"}

    def sprite_file(v):
        i = v >> 16
        if i not in files:
            files[i] = SpriteFile(i, v & 0xFFFF)
        return files[i]

    names = dw3.file_names()
    for key, (fname, table) in enumerate(sprites):
        if not fname or (only and not any(o.casefold() in fname.casefold() for o in only)):
            continue
        label = fname[:-4] + (" (jugador)" if key == PLAYER else "")
        if label not in chars:
            sets = []
            if key == PLAYER:
                for v in dict.fromkeys(PLAYER_FILES.values()):
                    f = sprite_file(v)
                    sets += [(s, f, dirs) for s, dirs in f.sets() if player_file(s) == v]
                sets.sort(key=lambda x: x[0])
            else:
                f = sprite_file(names.index(fname) << 16 | table)
                sets = [(s, f, dirs) for s, dirs in f.sets()]
            chars[label] = {"keys": [], "names": [], "maps": [], "sets": sets}
        c = chars[label]
        c["keys"].append(key)
        for x, _ in dw3.character_names(key):
            if x not in c["names"]:
                c["names"].append(x)
        if dw3.FIELD_PARTNERS <= key < dw3.FIELD_PARTNERS + 8:
            c["names"].insert(0, dw3.dname(key - dw3.FIELD_PARTNERS))
        for m in uses.get(key, []):
            if m not in c["maps"]:
                c["maps"].append(m)

    (out / "sheets").mkdir(parents=True, exist_ok=True)
    # each file's frames: those its sets' lists name
    sheet_frames = {}  # file index -> {TIM entry: frame number in its sheet}
    for c in chars.values():
        for _, f, dirs in c["sets"]:
            used = sheet_frames.setdefault(f.index, {})
            for e in dirs:
                for _, tim, _, _ in f.frames(e)[0]:
                    used.setdefault(tim, None)
    js_files = {}
    for i, used in sheet_frames.items():
        f = files[i]
        order = sorted(used)
        imgs = [f.tim(e) for e in order]
        places, size = pack([(w, h) for w, h, _ in imgs])
        sheet = Image.new("RGBA", size, (0, 0, 0, 0))
        for (x, y), (w, h, rgba) in zip(places, imgs):
            sheet.paste(Image.frombytes("RGBA", (w, h), rgba), (x, y))
        sheet.save(out / "sheets" / f"{f.name[:-4]}.png")
        for n, e in enumerate(order):
            used[e] = n
        js_files[f.name[:-4]] = {
            "sheet": f"sheets/{f.name[:-4]}.png",
            "frames": [[x, y, w, h, e] for (x, y), (w, h, _), e in zip(places, imgs, order)],
        }

    js_chars = []
    for label, c in sorted(chars.items(), key=lambda kv: kv[1]["keys"][0]):
        sets = []
        for s, f, dirs in c["sets"]:
            dl = []
            for e in dirs:
                steps, loops = f.frames(e)
                dl.append({"loop": loops, "steps": [[t, sheet_frames[f.index][tim], x, y] for t, tim, x, y in steps]})
            sets.append({"id": s, "name": SET_NAMES.get(s, ""), "file": f.name[:-4], "dirs": dl})
        js_chars.append({"label": label, "keys": c["keys"], "names": c["names"], "maps": c["maps"], "sets": sets})

    data = {"chars": js_chars, "files": js_files}
    (out / "data.js").write_text("window.SPRITES = " + json.dumps(data, ensure_ascii=False, separators=(",", ":"))
                                 + ";\n", encoding="utf-8")
    shutil.copyfile(VIEWER, out / "index.html")
    if gif:
        export_gifs(out, data)
    nframes = sum(len(v["frames"]) for v in js_files.values())
    print(f"{len(js_chars)} sprites, {len(js_files)} archivos, {nframes} fotogramas -> {out / 'index.html'}")


def export_gifs(out, data):
    """one GIF per set and direction (0-4; 5-7 are their mirrors), on a canvas that fits every frame"""
    sheets = {}
    for c in data["chars"]:
        for s in c["sets"]:
            f = data["files"][s["file"]]
            if s["file"] not in sheets:
                sheets[s["file"]] = Image.open(out / f["sheet"]).convert("RGBA")
            sheet = sheets[s["file"]]
            folder = out / "gif" / c["label"].split()[0]
            folder.mkdir(parents=True, exist_ok=True)
            for d, dl in enumerate(s["dirs"]):
                if not dl["steps"]:
                    continue
                boxes = [(x, y, x + f["frames"][n][2], y + f["frames"][n][3]) for _, n, x, y in dl["steps"]]
                x0, y0 = min(b[0] for b in boxes), min(b[1] for b in boxes)
                w, h = max(b[2] for b in boxes) - x0, max(b[3] for b in boxes) - y0
                frames, times = [], []
                for (t, n, x, y) in dl["steps"]:
                    fx, fy, fw, fh, _ = f["frames"][n]
                    img = Image.new("RGBA", (w, h), (0, 0, 0, 0))
                    img.paste(sheet.crop((fx, fy, fx + fw, fy + fh)), (x - x0, y - y0))
                    frames.append(img)
                    times.append(max(20, round(t * 1000 / 60)))
                frames[0].save(folder / f"{s['id']:02x}_{d}.gif", save_all=True, append_images=frames[1:],
                               duration=times, loop=0 if dl["loop"] else 1, disposal=2)


def main():
    ap = argparse.ArgumentParser(description="exporta los sprites de campo y sus animaciones para el visor")
    ap.add_argument("--out", type=Path, default=OUT, help=f"carpeta de salida (por defecto {OUT})")
    ap.add_argument("--gif", action="store_true", help="también un GIF por animación y dirección")
    ap.add_argument("--only", nargs="*", help="solo los archivos que contengan esto (P006AGUM, VMON...)")
    a = ap.parse_args()
    export(a.out, a.gif, a.only)


if __name__ == "__main__":
    main()
