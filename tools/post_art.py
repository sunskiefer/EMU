#!/usr/bin/env python3
"""EMU one-page skins: put the plugin's OWN panel art on the page and make the controls work on top of it.

    post_art.py <"Plugin Skins" dir> <layout.conf> <params.json> <spec.py>

  * sh_bg_0.png   = the supplied panel image, scaled and placed as spec ART says, on the theme background (a band stays free
                    at the bottom for the large readouts)
  * sh_knob_rNN   = rotating filmstrip made from that knob's own pixels in the panel image (rotated around its centre, so it
                    turns in place); sweep and the pointer's angle in the image come from the spec
  * buttons       = transparent when off, an orange highlight over the art while pressed; touch box sizes from the spec
  * the toggle    = the same, with its touch box grown to the spec's bw/bh; with spec tab= the switch's own tab is lifted out of the art and the
                    two toggle images draw it at the Saw (left) and Square (right) end
The art itself is never redrawn: every knob frame and the background are the supplied pixels.
"""
import json, os, re, runpy, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np, cv2
from PIL import Image

skin, layout, pj, specp = sys.argv[1:5]
root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
S = runpy.run_path(specp)["SPEC"]
A = S["art"]; sc = A["scale"]
art = np.array(Image.open(os.path.join(root, A["file"])).convert("RGBA"))
for (ex0, ey0, ex1, ey1, edy) in A.get("erase", []):             # parts of the art the owner asked to remove: cover with the empty panel dy px lower
    art[ey0:ey1, ex0:ex1] = art[ey0 + edy:ey1 + edy, ex0:ex1]
ACC = (240, 122, 30)

# ---- background
bg = Image.open(os.path.join(skin, "sh_bg_0.png")).convert("RGBA")
W, H = bg.size
th = S.get("theme", {}).get("bg", "1a1a1a")
base = Image.new("RGBA", (W, H), tuple(int(th[i:i + 2], 16) for i in (0, 2, 4)) + (255,))
a = Image.fromarray(art).resize((round(art.shape[1] * sc), round(art.shape[0] * sc)), Image.LANCZOS)
base.alpha_composite(a, (A["x"], A["y"]))
base.save(os.path.join(skin, "sh_bg_0.png"))


# ---- sliders and switches: lift each cap out of the art, then draw every frame with the cap moved along the track
def find_line(x, lo, hi):
    """y of the coloured indicator line on the cap near column x, searched between lo and hi (image pixels)."""
    seg = art[int(lo):int(hi), int(x) - 7:int(x) + 8, :3].astype(int)
    mx, mn = seg.max(2), seg.min(2)
    sat = ((mx - mn) > 0.45 * np.maximum(mx, 1)) & (mx > 110)
    rows = np.where(sat.sum(1) >= 7)[0]
    return None if len(rows) == 0 else float(np.median(rows)) + lo

sl = [c for c in S["controls"] if c["kind"] == "slider_v"]
art_clean = art.copy()
for c in sl:
    x = int(round(c["x"])); cw, ch = c["cap"]
    pts = c.get("pos") or [c["y0"], c["y1"]]
    ly = find_line(x, min(pts) - 22, max(pts) + 22)
    c["_line"] = ly
    if ly is None:
        print("post_art: NO CAP FOUND for", c["key"]); continue
    top, bot = int(round(ly - ch / 2 - 1)), int(round(ly + ch / 2 + 1))
    x0, x1 = x - cw // 2 - 3, x + cw // 2 + 4
    c["_sprite"] = art[top:bot + 1, x0:x1].copy(); c["_x0"] = x0; c["_top"] = top
    lum = art[..., :3].mean(2)
    donor = None
    for off in list(range(4, 14)) + [-o for o in range(4, 14)]:
        r = (top - off) if off > 0 else (bot - off)
        if 0 <= r < art.shape[0] and lum[r, x] < 70 and abs(r - ly) > ch / 2 + 2:
            donor = r; break
    if donor is None: donor = top - 6
    for r in range(top, bot + 1): art_clean[r, x0:x1] = art[donor, x0:x1]
    print("post_art: cap %-12s line y=%.1f cap %dx%d donor row %d" % (c["key"], ly, cw, ch, donor))

# ---- toggles with a sliding tab (spec: tab = dict(box=(x0,y0,x1,y1), donor_x, pos=(left_cx, right_cx)), image pixels): the tab is lifted out of the art,
# its old spot is covered with an empty strip of the same panel (donor_x), and the two toggle images draw the tab at each end
for c in S["controls"]:
    t = c.get("tab") if c["kind"] == "toggle" else None
    if not t: continue
    x0, y0, x1, y1 = t["box"]; dx = t["donor_x"]
    spr = art[y0:y1, x0:x1].astype(float)
    don = art[y0:y1, dx:dx + (x1 - x0)].astype(float)
    gs, gd = spr[..., :3].mean(2), np.maximum(don[..., :3].mean(2), 1.0)
    al = np.clip(1.0 - gs / gd, 0, 1)                                   # how much darker than the empty panel = opacity of a black layer
    al = np.where(al < 0.04, 0.0, al)
    col = np.clip((spr[..., :3] - (1 - al[..., None]) * don[..., :3]) / np.maximum(al[..., None], 1e-3), 0, 255)
    rgba = np.dstack([col, al * 255]).astype(np.uint8)
    t["_rgba"] = rgba
    art_clean[y0:y1, x0:x1] = art[y0:y1, dx:dx + (x1 - x0)]
    print("post_art: toggle tab %-10s lifted from %s, covered with strip at x=%d" % (c["key"], t["box"], dx))

# page background uses the cleaned art (replace the bg composited above)
base2 = Image.new("RGBA", (W, H), tuple(int(th[i:i + 2], 16) for i in (0, 2, 4)) + (255,))
base2.alpha_composite(Image.fromarray(art_clean).resize((round(art.shape[1] * sc), round(art.shape[0] * sc)), Image.LANCZOS), (A["x"], A["y"]))
base2.save(os.path.join(skin, "sh_bg_0.png"))

import glob
n_sl = 0
for fn in glob.glob(os.path.join(skin, "sh_slider_v_*.png")):
    m = re.match(r"sh_slider_v_(\d+)x(\d+)(?:_f(\d+))?\.png$", os.path.basename(fn))
    if not m: continue
    Wp, Hp = int(m.group(1)), int(m.group(2))
    srcs = [c for c in sl if round(c["w"] * sc) == Wp and round(c["h"] * sc) == Hp and c.get("_sprite") is not None]
    if not srcs: continue
    c = srcs[0]
    N = Image.open(fn).size[1] // Hp
    out = np.zeros((N * Hp, Wp, 4), np.uint8)
    x = c["x"]; hw, hh = c["w"], c["h"]
    ix0 = int(round(x - hw / 2)); iy0 = int(round(c["y"] - hh / 2))
    sp = c["_sprite"]; sh_, sw_ = sp.shape[:2]
    pts = c.get("pos") or [c["y0"], c["y1"]]
    for f in range(N):
        v = f / (N - 1)
        t = v * (len(pts) - 1); i = min(int(t), len(pts) - 2); cy = pts[i] + (pts[i + 1] - pts[i]) * (t - i)
        canvas = art_clean[iy0:iy0 + int(hh), ix0:ix0 + int(hw), :3].copy()
        sy = int(round(cy - c["_line"])) + c["_top"]          # where the sprite's top row lands
        sx = c["_x0"] - ix0
        for r in range(sh_):
            yy = sy + r - iy0
            if 0 <= yy < canvas.shape[0]:
                x_a, x_b = max(sx, 0), min(sx + sw_, canvas.shape[1])
                canvas[yy, x_a:x_b] = sp[r, x_a - sx:x_b - sx, :3]
        fr = cv2.resize(canvas.astype(np.uint8), (Wp, Hp), interpolation=cv2.INTER_AREA if Wp < canvas.shape[1] else cv2.INTER_CUBIC)
        out[f * Hp:(f + 1) * Hp, :, :3] = fr; out[f * Hp:(f + 1) * Hp, :, 3] = 255
    Image.fromarray(out).save(fn)
    n_sl += 1

# ---- knob filmstrips
def lerp_table(tab, v):
    t = v * (len(tab) - 1); i = min(int(t), len(tab) - 2); return tab[i] + (tab[i + 1] - tab[i]) * (t - i)

n_knob = 0
for c in S["controls"]:
    if c["kind"] != "knob" or c.get("skip_art"): continue
    fn = os.path.join(skin, "sh_knob_r%d.png" % c["r"])
    if not os.path.exists(fn) or c.get("_done"): continue
    sw, sh = Image.open(fn).size
    N = sh // sw
    out = np.zeros((sh, sw, 4), np.uint8)
    cx0, cy0, ir = float(c["x"]), float(c["y"]), float(c["ir"])
    base_ang = c.get("base", 0.0)
    for f in range(N):
        v = f / (N - 1)
        ang = lerp_table(c["angles"], v) if "angles" in c else c.get("a0", -145.0) + (c.get("a1", 145.0) - c.get("a0", -145.0)) * v
        if c.get("lit"):                                   # rendered knob: light fixed from above, only the knob's parts turn
            from knob_lit import frame as _lit
            out[f * sw:(f + 1) * sw] = _lit([x / 255.0 for x in c["lit"]], ang, sw); continue
        M = cv2.getRotationMatrix2D((cx0, cy0), -(ang - base_ang), sc)          # CW delta, source -> skin scale
        M[0, 2] += sw / 2 - cx0; M[1, 2] += sw / 2 - cy0
        fr = cv2.warpAffine(art, M, (sw, sw), flags=cv2.INTER_CUBIC, borderMode=cv2.BORDER_REPLICATE)
        yy, xx = np.mgrid[0:sw, 0:sw]
        d = np.hypot(xx + 0.5 - sw / 2, yy + 0.5 - sw / 2)
        al = np.clip(ir * sc - d + 0.5, 0, 1)
        fr[..., 3] = (al * 255).astype(np.uint8)
        out[f * sw:(f + 1) * sw] = fr
    Image.fromarray(out).save(fn)
    n_knob += 1

# ---- buttons and the toggle
specs = {c["key"]: c for c in S["controls"]}
tui_path = os.path.join(skin, "TUI.json")
d = json.load(open(tui_path))
defs = d["pageData"]["componentDefinitions"]["localComponentDefinitions"]

def hl(w, h, on, box=None):
    im = np.zeros((h, w, 4), np.uint8)
    if on:
        bw_, bh_ = box or (w, h)
        x0, y0 = (w - bw_) // 2, (h - bh_) // 2
        cv2.rectangle(im, (x0, y0), (x0 + bw_ - 1, y0 + bh_ - 1), ACC + (96,), -1)
        cv2.rectangle(im, (x0, y0), (x0 + bw_ - 1, y0 + bh_ - 1), ACC + (255,), 2)
    return Image.fromarray(im)

def tab_img(c, w, h, right):
    """The toggle image: the lifted tab drawn at its Saw (left) or Square (right) end, rest transparent. Drawn at image scale, then shrunk."""
    t = c["tab"]; x0, y0, x1, y1 = t["box"]; spr = t["_rgba"]
    W, H = round(w / sc), round(h / sc)
    ox, oy = c["x"] - W / 2.0, c["y"] - H / 2.0                          # image position of the image's top left
    cur = (x0 + x1) / 2.0
    tx = round(t["pos"][1 if right else 0] - cur + x0 - ox); ty = round(y0 - oy)
    cv = np.zeros((H, W, 4), np.uint8)
    sx0, sy0 = max(0, -tx), max(0, -ty)
    ex, ey = min(spr.shape[1], W - tx), min(spr.shape[0], H - ty)
    if ex > sx0 and ey > sy0: cv[ty + sy0:ty + ey, tx + sx0:tx + ex] = spr[sy0:ey, sx0:ex]
    im = Image.fromarray(cv, "RGBA").convert("RGBa").resize((w, h), Image.LANCZOS).convert("RGBA")
    return im

newsize = {}
keys_by_len = sorted((k for k, c in specs.items() if c["kind"] == "button"), key=len, reverse=True)
for e in defs:
    k = e["key"]
    if k.startswith("shTrig_"):
        rest = k[len("shTrig_"):]
        key = next(kk for kk in keys_by_len if rest == kk or rest.startswith(kk + "_"))
        c = specs[key]; w, h = c["bw"], c["bh"]
        slug = rest[len(key) + 1:]
        for on in (0, 1):
            hl(w, h, on, c.get("hl")).save(os.path.join(skin, "sh_btn_%s_%s_%s.png" % (key, slug, "on" if on else "off")))
        newsize[k] = (w, h)
    elif k.startswith("shToggle"):
        c = next(c for c in S["controls"] if c["kind"] == "toggle")
        w, h = c["bw"], c.get("bh", 56)
        for on in (0, 1): (tab_img(c, w, h, on) if c.get("tab") else hl(w, h, on)).save(os.path.join(skin, "sh_pill_%s.png" % ("on" if on else "off")))
        newsize[k] = (w, h)
    else:
        continue
    for comp in e["value"]["componentsData"]:
        b = comp["bounds"]
        if b.get("bounds") and comp["componentData"]["type"] in ("Focus", "Button"):
            b["bounds"] = "0 0 %d %d" % newsize[k]

def walk(o):
    if isinstance(o, dict):
        t = o.get("componentData", {}).get("type") if "componentData" in o else None
        if t in newsize and "bounds" in o and o["bounds"].get("bounds"):
            x, y, w, h = [int(float(v)) for v in o["bounds"]["bounds"].split()]
            nw, nh = newsize[t]
            o["bounds"]["bounds"] = "%d %d %d %d" % (x + w // 2 - nw // 2, y + h // 2 - nh // 2, nw, nh)
        for v in o.values(): walk(v)
    elif isinstance(o, list):
        for v in o: walk(v)
walk(d["pageData"])
json.dump(d, open(tui_path, "w"))
print("post_art: background + %d knob filmstrips + %d slider filmstrips + %d buttons/toggle" % (n_knob, n_sl, len(newsize)))
