#!/usr/bin/env python3
"""EMU: redraw the skin's buttons, overview tiles and option segments with the standing L'Cronx text sizes.

    post_skin.py <skin dir ("Plugin Skins")> <layout.conf> <params.json>

shadow_art draws its buttons in a tiny bitmap font. Every EMU button, tile and option segment is redrawn here in
Titillium Web SemiBold at 30 px or larger (never smaller than the Meta4MPCXXL sizes), and its touch box is grown to fit,
keeping its centre where the layout put it.
"""
import json, os, re, sys
from PIL import Image, ImageDraw, ImageFont

skin, layout, pj = sys.argv[1:4]
FONT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "art", "fonts", "TitilliumWeb-SemiBold.ttf")
ACCENT, ON_TX, OFF_FILL, OFF_LINE, INK = (240, 122, 30), (17, 17, 17), (42, 42, 42), (96, 96, 96), (242, 242, 242)
labels = {}
for line in open(layout):
    m = re.match(r'button .*label="([^"]*)" key=(\w+)', line)
    if m: labels[m.group(2)] = m.group(1)
opts = {p["key"]: p.get("options") for p in json.load(open(pj))["params"]}

def f(size): return ImageFont.truetype(FONT, size)

def draw(w, h, text, on, size, radius=10):
    im = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    d = ImageDraw.Draw(im)
    d.rounded_rectangle((0, 0, w - 1, h - 1), radius, fill=ACCENT + (255,) if on else OFF_FILL + (255,),
                        outline=ACCENT + (255,) if on else OFF_LINE + (255,), width=2)
    ft = f(size)
    d.text((w / 2, h / 2), text, font=ft, fill=(ON_TX if on else INK) + (255,), anchor="mm")
    return im

def fit_size(text, w, start, floor=26):
    s = start
    while s > floor and f(s).getlength(text) > w - 24: s -= 1
    return s

tui_path = os.path.join(skin, "TUI.json")
d = json.load(open(tui_path))
defs = d["pageData"]["componentDefinitions"]["localComponentDefinitions"]
newsize = {}
changed = 0
for e in defs:
    k = e["key"]
    m = re.match(r"shTrig_(\w+?)_([^_]+)$", k)
    seg = re.match(r"shSeg_(\w+)_(\d+)$", k)
    if m and not seg:
        key = m.group(1)
        text = labels.get(key, m.group(2))
        tile = key.startswith("go_")
        size = 44 if tile else 34
        w0 = max(int(f(size).getlength(text)) + 56, 150 if tile else 96)
        if tile: w0, h = 360, 170
        else: h = 64
        w0 = max(w0, 96)
        for on in (0, 1):
            draw(w0, h, text, on, size).save(os.path.join(skin, "sh_btn_%s_%s_%s.png" % (key, m.group(2), "on" if on else "off")))
        newsize[k] = (w0, h)
        for c in e["value"]["componentsData"]:
            b = c["bounds"]
            if b.get("bounds") and c["componentData"]["type"] in ("Focus", "Button"):
                b["bounds"] = "0 0 %d %d" % (w0, h)
        changed += 1
    elif seg:
        key, i = seg.group(1), int(seg.group(2))
        o = opts.get(key)
        if not o: continue
        text = o[i].upper()
        for c in e["value"]["componentsData"]:
            if c["componentData"]["type"] == "Button":
                dat = c["componentData"]["data"]
                for on, nm in ((0, dat["offImage"]), (1, dat["onImage"])):
                    p = os.path.join(skin, nm)
                    w, h = Image.open(p).size
                    draw(w, h, text, on, fit_size(text, w, 34), 6).save(p)
        changed += 1
# instances: grow the touch box, keep the centre
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
print("post_skin: %d button/segment definitions redrawn" % changed)
