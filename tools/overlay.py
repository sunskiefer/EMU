#!/usr/bin/env python3
"""Numbered overlay of every control on the page preview, plus the control map table.

    overlay.py <plugin folder>      (needs <plugin>/build/preview/p0.png from studio.py preview)
Writes build/preview/overlay.png and control_map.txt (id, type, key, page x/y, touch box). The id is the line order in layout.conf.
"""
import os, re, sys
from PIL import Image, ImageDraw, ImageFont
d = sys.argv[1]
im = Image.open(os.path.join(d, "build/preview/p0.png")).convert("RGB"); dr = ImageDraw.Draw(im)
f = ImageFont.truetype(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "art/fonts/TitilliumWeb-SemiBold.ttf"), 14)
rows = []
for l in open(os.path.join(d, "layout.conf")):
    m = re.match(r"(knob|button|toggle|readout|slider_v) cx=(\d+) cy=(\d+)(.*)", l)
    if not m: continue
    k, cx, cy, rest = m.group(1), int(m.group(2)), int(m.group(3)) - 86, m.group(4)
    key = re.search(r"key=(\w+)", rest).group(1); lab = re.search(r'label="([^"]*)"', rest).group(1)
    g = lambda n, dflt=0: int(re.search(r"\b%s=(\d+)" % n, rest).group(1)) if re.search(r"\b%s=(\d+)" % n, rest) else dflt
    n = len(rows) + 1; box = ""
    if k == "knob":
        r = g("r"); dr.ellipse((cx - r, cy - r, cx + r, cy + r), outline=(255, 0, 0), width=2); box = "r=%d" % r
    elif k == "readout": rows.append((n, k, key, lab, cx, cy + 86, "w=%d h=%d" % (g("w"), g("h")))); continue
    else:
        bw = g("bw", g("w")); bh = g("bh", g("h", 56)); dr.rectangle((cx - bw // 2, cy - bh // 2, cx + bw // 2, cy + bh // 2), outline=(0, 160, 255), width=1); box = "%dx%d" % (bw, bh)
    dr.text((cx - 6, cy - 8), str(n), fill=(255, 255, 0), font=f, stroke_width=2, stroke_fill=(0, 0, 0))
    rows.append((n, k, key, lab, cx, cy + 86, box))
im.save(os.path.join(d, "build/preview/overlay.png"))
open(os.path.join(d, "control_map.txt"), "w").write("id\ttype\tkey\tlabel\tx(layout)\ty(layout)\ttouch box\n" + "\n".join("\t".join(map(str, r)) for r in rows) + "\n")
print(len(rows), "controls")
