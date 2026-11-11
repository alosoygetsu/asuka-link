# Asuka Link's icon picture (1024 px) for tools/make_icon.py: a sunset over two towers linked, and "AL" in pixel letters
import sys
from PIL import Image, ImageDraw

W = 1024
img = Image.new("RGB", (W, W), (5, 11, 20))
d = ImageDraw.Draw(img)
X0, Y0, X1, Y1 = 76, 76, 948, 946
# the sky: night blue to dusk rose
for y in range(Y0, Y1):
    t = (y - Y0) / (Y1 - Y0)
    stops = [(0, (8, 17, 28)), (0.45, (30, 52, 92)), (0.7, (106, 58, 106)), (1, (196, 86, 106))]
    for (a, ca), (b, cb) in zip(stops, stops[1:]):
        if a <= t <= b:
            k = (t - a) / (b - a)
            c = tuple(int(ca[i] + (cb[i] - ca[i]) * k) for i in range(3))
    d.line([(X0, y), (X1, y)], fill=c)
# the sun with stripes
cx, cy, r = 512, 700, 230
d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(255, 196, 106))
d.ellipse([cx - r + 40, cy - r + 30, cx + r - 70, cy + r - 90], fill=(255, 230, 176))
y, gap = cy + 20, 8
while y < cy + r:
    d.rectangle([cx - r, y, cx + r, y + gap // 2], fill=(106, 58, 106))
    y += 34
    gap += 6
# the city
city = (11, 24, 38)
d.rectangle([X0, 830, X1, Y1], fill=city)
for x, w, h in [(120, 90, 160), (230, 70, 110), (640, 80, 130), (740, 70, 90)]:
    d.rectangle([x, 830 - h, x + w, 830], fill=city)
# the two linked towers
for x, h in [(170, 420), (780, 500)]:
    d.rectangle([x - 40, 830 - h, x + 40, 830], fill=city)
    d.rectangle([x - 6, 830 - h - 110, x + 6, 830 - h], fill=city)
    d.ellipse([x - 22, 830 - h - 132, x + 22, 830 - h - 88], fill=(92, 230, 255))
a, b = (170, 830 - 420 - 110), (780, 830 - 500 - 110)
pts = []
for i in range(41):
    t = i / 40
    pts.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t - 110 * 4 * t * (1 - t)))
for i, p in enumerate(pts):
    if i % 2 == 0:
        d.rectangle([p[0] - 9, p[1] - 9, p[0] + 9, p[1] + 9], fill=(92, 230, 255))
# AL in pixel letters with a dark edge
A = ["01110", "10001", "10001", "11111", "10001", "10001", "10001"]
L = ["10000", "10000", "10000", "10000", "10000", "10000", "11111"]
cell, top = 44, 330
for g, glyph in enumerate([A, L]):
    left = 250 + g * 6 * cell
    for row, line in enumerate(glyph):
        for col, ch in enumerate(line):
            if ch == "1":
                x, y = left + col * cell, top + row * cell
                d.rectangle([x - 12, y - 12, x + cell + 12, y + cell + 12], fill=(5, 11, 20))
    for row, line in enumerate(glyph):
        for col, ch in enumerate(line):
            if ch == "1":
                x, y = left + col * cell, top + row * cell
                t = row / 6
                d.rectangle([x, y, x + cell, y + cell], fill=(255, int(216 - t * 128), int(122 - t * 80)))
img.save(sys.argv[1])
