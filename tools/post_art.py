#!/usr/bin/env python3
"""EMU one-page skins: put the plugin's OWN panel art on the page and make the controls work on top of it.

    post_art.py <"Plugin Skins" dir> <layout.conf> <params.json> <spec.py>

  * sh_bg_0.png   = the supplied panel image, scaled and placed as spec ART says, on the theme background (a band stays free
                    at the bottom for the large readouts)
  * sh_knob_rNN   = rotating filmstrip made from that knob's own pixels in the panel image (rotated around its centre, so it
                    turns in place); sweep and the pointer's angle in the image come from the spec
  * buttons       = transparent when off, an orange highlight over the art while pressed; touch box sizes from the spec
  * the toggle    = the same, with its touch box grown to the spec's bw/bh
The art itself is never redrawn: every knob frame and the background are the supplied pixels.
"""
import json, os, re, runpy, sys
import numpy as np, cv2
from PIL import Image

skin, layout, pj, specp = sys.argv[1:5]
root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
S = runpy.run_path(specp)["SPEC"]
A = S["art"]; sc = A["scale"]
art = np.array(Image.open(os.path.join(root, A["file"])).convert("RGBA"))
ACC = (240, 122, 30)

# ---- background
bg = Image.open(os.path.join(skin, "sh_bg_0.png")).convert("RGBA")
W, H = bg.size
th = S.get("theme", {}).get("bg", "1a1a1a")
base = Image.new("RGBA", (W, H), tuple(int(th[i:i + 2], 16) for i in (0, 2, 4)) + (255,))
a = Image.fromarray(art).resize((round(art.shape[1] * sc), round(art.shape[0] * sc)), Image.LANCZOS)
base.alpha_composite(a, (A["x"], A["y"]))
base.save(os.path.join(skin, "sh_bg_0.png"))

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

def hl(w, h, on):
    im = np.zeros((h, w, 4), np.uint8)
    if on:
        im[..., :3] = ACC; im[..., 3] = 96
        cv2.rectangle(im, (0, 0), (w - 1, h - 1), ACC + (255,), 3)
    return Image.fromarray(im)

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
            hl(w, h, on).save(os.path.join(skin, "sh_btn_%s_%s_%s.png" % (key, slug, "on" if on else "off")))
        newsize[k] = (w, h)
    elif k.startswith("shToggle"):
        c = next(c for c in S["controls"] if c["kind"] == "toggle")
        w, h = c["bw"], c.get("bh", 56)
        for on in (0, 1): hl(w, h, on).save(os.path.join(skin, "sh_pill_%s.png" % ("on" if on else "off")))
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
print("post_art: background + %d knob filmstrips + %d buttons/toggle" % (n_knob, len(newsize)))
