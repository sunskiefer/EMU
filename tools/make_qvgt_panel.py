#!/usr/bin/env python3
"""PLACEHOLDER panel for QVRBGT MPC (the user has not supplied a QuadraVerb GT panel image): plain boxes, labels and two knobs, 1180 x 568.
Replace art/panels/qvgt.png with the real panel and re-point the x/y in QVRBGTMPC/spec.py; nothing else depends on this drawing."""
import math, os
from PIL import Image, ImageDraw, ImageFont
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 1180, 568
im = Image.new("RGB", (W, H), (43, 45, 49)); d = ImageDraw.Draw(im)
F = lambda s: ImageFont.truetype(os.path.join(root, "art/fonts/TitilliumWeb-SemiBold.ttf"), s)
d.rectangle((0, 0, W - 1, H - 1), outline=(90, 92, 98), width=4)
d.text((60, 14), "QVRBGT MPC", font=F(26), fill=(240, 122, 30))
# LCD bezel (the two text lines are live readouts on top of it)
d.rounded_rectangle((50, 56, 770, 246), 14, fill=(14, 22, 14), outline=(90, 92, 98), width=3)
# INPUT / OUTPUT knobs: dark disc, ring of ticks, orange pointer straight up
def knob(cx, cy, label):
    for a in range(-150, 151, 30):
        x0 = cx + math.sin(math.radians(a)) * 66; y0 = cy - math.cos(math.radians(a)) * 66
        x1 = cx + math.sin(math.radians(a)) * 78; y1 = cy - math.cos(math.radians(a)) * 78
        d.line((x0, y0, x1, y1), fill=(190, 190, 190), width=3)
    d.ellipse((cx - 54, cy - 54, cx + 54, cy + 54), fill=(24, 24, 26), outline=(70, 72, 76), width=4)
    d.ellipse((cx - 40, cy - 40, cx + 40, cy + 40), fill=(34, 34, 37))
    d.line((cx, cy - 12, cx, cy - 50), fill=(240, 122, 30), width=7)
    t = F(26); w = d.textlength(label, font=t); d.text((cx - w / 2, cy + 84), label, font=t, fill=(220, 220, 220))
knob(895, 150, "INPUT"); knob(1060, 150, "OUTPUT")
# the 16 keys, two rows of eight (the key boxes are the touch targets; labels are drawn here)
ROW1 = ["PROGRAM", "EQ", "PRE-AMP", "PITCH", "DELAY", "REVERB", "MIX", "CONFIG"]
ROW2 = ["VALUE -", "VALUE +", "PAGE -", "PAGE +", "STORE", "NAME/MOD", "MIDI", "BYPASS"]
for r, (row, y) in enumerate(((ROW1, 330), (ROW2, 450))):
    for i, name in enumerate(row):
        x = 64 + i * 137
        d.rounded_rectangle((x, y - 42, x + 118, y + 42), 10, fill=(58, 60, 66), outline=(120, 122, 128), width=3)
        t = F(24); w = d.textlength(name, font=t); d.text((x + 59 - w / 2, y - 16), name, font=t, fill=(235, 235, 235))
os.makedirs(os.path.join(root, "art/panels"), exist_ok=True)
im.save(os.path.join(root, "art/panels/qvgt.png"))
print("art/panels/qvgt.png", im.size)
