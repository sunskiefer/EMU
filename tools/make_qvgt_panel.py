#!/usr/bin/env python3
"""Hardware-style front panel for QVRBGT MPC (1180 x 568), drawn from scratch at 2x and scaled down: anodised brushed aluminium,
Phillips screws with shadows, bevelled recessed LCD window with a 2 x 16 dot-matrix grid under green backlight, knob wells with
scale ticks, bevelled rubber keys with unlit LEDs, silk-screen legends. NO manufacturer logo or name. The user has not supplied a
real QuadraVerb GT panel; replace art/panels/qvgt.png with theirs and re-point the x/y in QVRBGTMPC/spec.py.
Control geometry is unchanged: knobs (895,150) and (1060,150); keys left x=64+137*i, y=330/450 (118 x 84 pads in 130 x 88 boxes);
LCD inner window (52,60)-(768,242)."""
import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from scipy.ndimage import uniform_filter1d

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H, S = 1180, 568, 2
W2, H2 = W * S, H * S
rng = np.random.default_rng(11)
FONT = lambda s, w="SemiBold": ImageFont.truetype(os.path.join(root, "art/fonts/TitilliumWeb-SemiBold.ttf"), int(s * S))
q = lambda v: int(round(v * S))
Q = lambda box: tuple(q(v) for v in box)

# ---- brushed anodised aluminium (long horizontal grain, soft top light, vignette) ----
g = rng.normal(0, 1, (H2, W2)).astype(np.float32)
g = uniform_filter1d(g, 90, axis=1); g /= g.std()
g2 = uniform_filter1d(rng.normal(0, 1, (H2, W2)).astype(np.float32), 14, axis=1); g2 /= g2.std()
fine = rng.normal(0, 1, (H2, W2)).astype(np.float32)
yy = np.linspace(0, 1, H2, dtype=np.float32)[:, None]
xx = np.linspace(0, 1, W2, dtype=np.float32)[None, :]
base = 176 - 40 * yy + 14 * np.exp(-((yy - 0.04) / 0.06) ** 2) + 8 * np.exp(-((yy - 0.52) / 0.05) ** 2)             # lighter at the top edge
base = base - 22 * (np.abs(xx - 0.5) * 2) ** 3                            # vignette at the sides
lum = base + 5.0 * g + 2.4 * g2 + 1.2 * fine
rgb = np.stack([lum * 0.975, lum * 0.99, lum * 1.03], -1)
im = Image.fromarray(np.clip(rgb, 0, 255).astype(np.uint8), "RGB").convert("RGBA")


def shadow(draw_fn, off=(0, 4), blur=5, alpha=150, grow=0):
    """Composite a blurred drop shadow of whatever draw_fn paints (white) onto the panel."""
    m = Image.new("L", im.size, 0)
    draw_fn(ImageDraw.Draw(m), 255)
    if grow:
        m = m.filter(ImageFilter.MaxFilter(grow * 2 + 1))
    m = m.filter(ImageFilter.GaussianBlur(q(blur)))
    sh = Image.new("RGBA", im.size, (0, 0, 0, 0))
    sh.putalpha(m.point(lambda v: int(v * alpha / 255)))
    layer = Image.new("RGBA", im.size, (0, 0, 0, 0))
    layer.alpha_composite(sh, (q(off[0]), q(off[1])))
    im.alpha_composite(layer)


def vgrad(box, r, top, bot, extra=None):
    """Rounded rectangle filled with a vertical gradient."""
    x0, y0, x1, y1 = Q(box)
    w, h = x1 - x0, y1 - y0
    t = np.linspace(0, 1, h, dtype=np.float32)[:, None, None]
    arr = np.array(top, np.float32) * (1 - t) + np.array(bot, np.float32) * t
    arr = np.repeat(arr, w, axis=1)
    if extra is not None:
        arr = arr + extra(w, h)
    tile = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    m = Image.new("L", (w, h), 0)
    ImageDraw.Draw(m).rounded_rectangle((0, 0, w - 1, h - 1), q(r), fill=255)
    tile.putalpha(m)
    im.alpha_composite(tile, (x0, y0))


def radial_disc(cx, cy, rad, c_in, c_out, hl=(-0.3, -0.35)):
    """Disc with an off-centre radial gradient (screws, lamps)."""
    n = q(rad) * 2 + 2
    ys, xs = np.mgrid[0:n, 0:n].astype(np.float32)
    c = n / 2
    d = np.hypot(xs - c - hl[0] * q(rad), ys - c - hl[1] * q(rad)) / q(rad)
    t = np.clip(d / 1.25, 0, 1)[..., None]
    arr = np.array(c_in, np.float32) * (1 - t) + np.array(c_out, np.float32) * t
    tile = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGB").convert("RGBA")
    m = Image.new("L", (n, n), 0)
    ImageDraw.Draw(m).ellipse((c - q(rad), c - q(rad), c + q(rad), c + q(rad)), fill=255)
    tile.putalpha(m)
    im.alpha_composite(tile, (q(cx) - n // 2, q(cy) - n // 2))


def D():
    return ImageDraw.Draw(im)


def centered(text, cx, y, font, fill):
    d = D(); w = d.textlength(text, font=font); d.text((q(cx) - w / 2, q(y)), text, font=font, fill=fill)


def engrave(text, cx, y, font, fill=(26, 27, 31)):
    """Silk-screen text with a faint dark under-print, so it reads as printed on the metal."""
    d = D(); w = d.textlength(text, font=font)
    d.text((q(cx) - w / 2 + 1, q(y) + 2), text, font=font, fill=(236, 239, 245))
    d.text((q(cx) - w / 2, q(y)), text, font=font, fill=fill)


# ---- frame: bevelled edge, inner rule ----
d = D()
d.rectangle((0, 0, W2 - 1, H2 - 1), outline=(14, 15, 18), width=q(3))
d.rectangle((q(3), q(3), W2 - q(3) - 1, H2 - q(3) - 1), outline=(236, 239, 245), width=q(1.5))
d.rectangle((q(5), q(5), W2 - q(5) - 1, H2 - q(5) - 1), outline=(70, 73, 80), width=q(1))

# ---- screws (Phillips, random drive angle, drop shadow) ----
for sx, sy in ((26, 26), (W - 27, 26), (26, H - 27), (W - 27, H - 27)):
    shadow(lambda dd, c, sx=sx, sy=sy: dd.ellipse(Q((sx - 11, sy - 11, sx + 11, sy + 11)), fill=c), off=(1.5, 3), blur=2.5, alpha=190)
    radial_disc(sx, sy, 11, (178, 182, 190), (62, 65, 72))
    radial_disc(sx, sy, 8, (150, 154, 162), (88, 91, 98), hl=(-0.2, -0.25))
    a = math.radians(rng.uniform(0, 90)); d = D()
    for k in (0, 90):
        ax = math.cos(a + math.radians(k)); ay = math.sin(a + math.radians(k))
        d.line((q(sx - 6 * ax), q(sy - 6 * ay), q(sx + 6 * ax), q(sy + 6 * ay)), fill=(30, 31, 35), width=q(2.6))
        d.line((q(sx - 6 * ax + 0.8), q(sy - 6 * ay + 1), q(sx + 6 * ax + 0.8), q(sy + 6 * ay + 1)), fill=(170, 174, 182), width=q(0.7))

# ---- title strip (plugin's own name; no maker) ----
engrave("QVRBGT MPC", 150, 12, FONT(30, "Bold"), (196, 66, 18))
engrave("DIGITAL MULTI-EFFECTS PROCESSOR", 540, 19, FONT(18), (56, 59, 66))

# ---- LCD: bezel, black glass, 2 x 16 dot-matrix grid under green backlight ----
bez = (42, 50, 778, 252)
shadow(lambda dd, c: dd.rounded_rectangle(Q(bez), q(13), fill=c), off=(0, 5), blur=6, alpha=200)
vgrad(bez, 13, (22, 23, 26), (112, 116, 124))                    # recess: dark on top, light lip at the bottom
vgrad((46, 54, 774, 248), 10, (6, 7, 8), (28, 30, 34))
LX0, LY0, LX1, LY1 = 52, 60, 768, 242
lw, lh = q(LX1 - LX0), q(LY1 - LY0)
ly, lx = np.mgrid[0:lh, 0:lw].astype(np.float32)
glow = 0.62 + 0.38 * (1 - np.abs(ly / lh - 0.5) * 1.5) * (1 - 0.35 * np.abs(lx / lw - 0.5) * 2)
lcd = np.zeros((lh, lw, 3), np.float32)
lcd[..., 0] = 24 * glow; lcd[..., 1] = 64 * glow; lcd[..., 2] = 26 * glow
lcd_img = Image.fromarray(np.clip(lcd, 0, 255).astype(np.uint8), "RGB")
ld = ImageDraw.Draw(lcd_img)
pitch = 7.0
cols = 16; cell_w = (LX1 - LX0 - 24) / cols                       # 16 characters of 5 x 7 dots
for row_cy in (110 - LY0, 190 - LY0):                             # the two text lines sit on these centres
    for c in range(cols):
        for i in range(5):
            for j in range(7):
                px = 12 + c * cell_w + (cell_w - 5 * pitch) / 2 + i * pitch + pitch / 2
                py = row_cy - 3 * pitch + j * pitch
                ld.ellipse((q(px - 2.1), q(py - 2.1), q(px + 2.1), q(py + 2.1)), fill=(12, 46, 16))
im.paste(lcd_img, (q(LX0), q(LY0)))
sheen = Image.new("RGBA", (lw, lh), (0, 0, 0, 0)); sp = ImageDraw.Draw(sheen)
sp.polygon([(0, 0), (int(lw * 0.62), 0), (int(lw * 0.40), lh * 0.55), (0, lh * 0.55)], fill=(255, 255, 255, 14))
sheen = sheen.filter(ImageFilter.GaussianBlur(q(6)))
im.alpha_composite(sheen, (q(LX0), q(LY0)))
d = D(); d.rounded_rectangle(Q((LX0 - 1, LY0 - 1, LX1 + 1, LY1 + 1)), q(8), outline=(0, 0, 0), width=q(3))
d.rounded_rectangle(Q((LX0 + 1, LY0 + 1, LX1 - 1, LY1 - 1)), q(7), outline=(40, 44, 40), width=q(1))

# ---- input / output block: engraved box, knob wells, scale ticks ----
io = (800, 44, 1140, 264)
d = D(); d.rounded_rectangle(Q(io), q(10), outline=(60, 63, 70), width=q(2))
d.rounded_rectangle(Q((io[0] + 1, io[1] + 2, io[2] + 1, io[3] + 2)), q(10), outline=(244, 246, 250), width=q(1))
for cx, cy, label in ((895, 150, "INPUT"), (1060, 150, "OUTPUT")):
    shadow(lambda dd, c, cx=cx, cy=cy: dd.ellipse(Q((cx - 72, cy - 72, cx + 72, cy + 72)), fill=c), off=(0, 3), blur=4, alpha=170)
    radial_disc(cx, cy, 72, (20, 21, 24), (96, 100, 108), hl=(0, 0.55))     # chamfered well: shaded top, lit lip below
    radial_disc(cx, cy, 66, (6, 6, 7), (22, 23, 26), hl=(0, -0.2))
    d = D()
    for k, a in enumerate(range(-150, 151, 15)):
        major = k % 2 == 0
        r0, r1 = (72, 81) if major else (72, 77)
        sx, sy = math.sin(math.radians(a)), -math.cos(math.radians(a))
        d.line((q(cx + sx * r0 + 0.8), q(cy + sy * r0 + 1.2), q(cx + sx * r1 + 0.8), q(cy + sy * r1 + 1.2)), fill=(246, 248, 252), width=q(2.8 if major else 1.8))
        d.line((q(cx + sx * r0), q(cy + sy * r0), q(cx + sx * r1), q(cy + sy * r1)), fill=(24, 25, 29), width=q(2.4 if major else 1.4))
    engrave(label, cx, 222, FONT(26, "Bold"))

# ---- engraved rule between display/levels and the keys ----
d = D(); d.line((q(44), q(272), q(W - 44), q(272)), fill=(70, 73, 80), width=q(2))
d.line((q(44), q(274), q(W - 44), q(274)), fill=(246, 248, 252), width=q(1))
t = FONT(17); tl = d.textlength("EDIT   AND   STORE", font=t); x0, x1, y = 64, 64 + 7 * 137 + 118, 396
for xa, xb in ((x0, (x0 + x1) / 2 - tl / S / 2 - 10), ((x0 + x1) / 2 + tl / S / 2 + 10, x1)):
    d.line((q(xa), q(y + 1), q(xb), q(y + 1)), fill=(244, 246, 250), width=q(2)); d.line((q(xa), q(y), q(xb), q(y)), fill=(40, 42, 48), width=q(1.4))
for xe in (x0, x1):
    d.line((q(xe), q(y), q(xe), q(y + 9)), fill=(40, 42, 48), width=q(1.4))
engrave("EDIT   AND   STORE", (x0 + x1) / 2, y - 12, t, (40, 42, 48))

# ---- 16 keys: bevelled rubber pads, shadowed wells, unlit LED, printed legends ----
ROW1 = ["PROGRAM", "EQ", "PRE-AMP", "PITCH", "DELAY", "REVERB", "MIX", "CONFIG"]
ROW2 = ["VALUE -", "VALUE +", "PAGE -", "PAGE +", "STORE", "NAME/MOD", "MIDI", "BYPASS"]
grain = lambda w, h: rng.normal(0, 1.4, (h, w, 1)).astype(np.float32)
for row, y in ((ROW1, 330), (ROW2, 450)):
    for i, name in enumerate(row):
        x = 64 + i * 137
        tf = FONT(22 if len(name) > 8 else 24)
        engrave(name, x + 59, y - 44, tf)                                            # legend printed on the faceplate
        pad = (x, y - 8, x + 118, y + 42)
        well = (x - 4, y - 12, x + 122, y + 46)
        shadow(lambda dd, c, well=well: dd.rounded_rectangle(Q(well), q(12), fill=c), off=(0, 2), blur=2.5, alpha=120)
        vgrad(well, 12, (64, 66, 72), (246, 248, 252))                               # machined opening: dark top wall, bright lower lip
        vgrad((x - 2, y - 10, x + 120, y + 44), 10, (6, 6, 7), (16, 17, 19))
        shadow(lambda dd, c, pad=pad: dd.rounded_rectangle(Q(pad), q(9), fill=c), off=(0, 3), blur=3, alpha=210)
        vgrad(pad, 9, (74, 76, 82), (22, 23, 26), extra=grain)
        vgrad((x + 1.5, y - 6.5, x + 116.5, y + 30), 8, (92, 94, 101), (40, 41, 45), extra=grain)
        d = D()
        d.rounded_rectangle(Q((x + 1, y - 7, x + 117, y + 41)), q(9), outline=(158, 162, 170), width=q(1.2))
        d.line((q(x + 8), q(y + 36), q(x + 110), q(y + 36)), fill=(14, 15, 17), width=q(2))
        radial_disc(x + 106, y + 2.5, 3.8, (150, 52, 46), (52, 15, 15), hl=(-0.3, -0.35))   # unlit LED
        d = D(); d.ellipse(Q((x + 102.6, y - 1, x + 109.4, y + 6)), outline=(10, 10, 12), width=q(1))

# fine film grain over everything so it does not look like flat vector art
arr = np.asarray(im.convert("RGB"), np.float32) + rng.normal(0, 1.1, (H2, W2, 1))
out = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), "RGB").resize((W, H), Image.LANCZOS)
os.makedirs(os.path.join(root, "art/panels"), exist_ok=True)
out.save(os.path.join(root, "art/panels/qvgt.png"))
print("art/panels/qvgt.png", out.size)
