# 303MPC: ONE page over the user's own panel art (art/panels/303.png, 1858 x 846, supplied 2026-11-17, used as is).
# x, y are PIXELS OF THAT IMAGE (measured with code, see 303MPC/control_map.txt). gen_emu.py turns them into page coordinates:
#   cx = 16 + x*0.6714,  cy = y*0.6714 + 86.
# To move a control, edit its x/y here and rebuild. ir = the knob's radius in the image; r = its radius on the page (it also names
# its filmstrip, so every knob has its own r). bw/bh = touch box on the page; hl = size of the press highlight on the page.
# base = angle (deg clockwise from 12 o'clock) of the pointer end of the knob as it sits in the art (right-hand end of the bar).
# erase = socket labels you asked to delete (MIX IN, SYNC IN, CV / GATE / HEADPHONE / OUTPUT / DC 9V): (x0, y0, x1, y1, dy) in image pixels, covered with the
# empty panel dy pixels lower. Nothing else on the panel is touched.
ART = dict(file="art/panels/303.png", scale=568 / 846, x=round((1280 - 1858 * 568 / 846) / 2), y=0,
           erase=[(57, 11, 113, 51, 64), (325, 11, 396, 51, 64), (1401, 11, 1816, 51, 64)])
K = lambda key, name, x, y, ir, r, d=5, base=0: dict(key=key, name=name, kind="knob", x=x, y=y, ir=ir, r=r, min=0, max=10, default=d, bw=84, base=base)
# note and function buttons: x, y = centre of the cap in the image, cw/ch = cap size in the image (the press highlight), bw/bh = touch box on the page
def B(key, name, x, y, cw, ch, bw=None, bh=None):
    return dict(key=key, name=name, kind="button", x=x, y=y, bw=bw or max(48, round(cw * 0.6714) + 14), bh=bh or max(48, round(ch * 0.6714) + 10),
                hl=(round(cw * 0.6714), round(ch * 0.6714)))
C = [
 # knobs: x, y, ir = centre and radius measured on the panel; base = pointer angle measured on the panel (bar axis, right-hand end)
 K("tuning", "TUNING", 558.3, 90.8, 42, 29, 5, 136.7), K("cutoff", "CUT OFF", 707.7, 91.1, 42, 30, 5, 136.2), K("resonance", "RES", 853.0, 91.1, 42, 31, 5, 144.5),
 K("envmod", "ENV MOD", 997.0, 91.5, 42, 32, 5, 138.2), K("decay", "DECAY", 1140.5, 91.3, 42, 33, 5, 136.6), K("accent", "ACCENT", 1285.5, 91.5, 42, 34, 5, 138.3),
 K("tempo", "TEMPO", 257.9, 288.6, 65.3, 45, 5, 140.9), K("volume", "VOLUME", 1634.9, 288.8, 65.7, 47, 7, 140.5),
 dict(key="track", name="PATT GROUP", kind="knob", x=555.9, y=289.1, ir=64.3, r=44, min=0, max=6, default=0, int=True, options=None, bw=100, base=137.4,
      angles=[-105, -78, -50, -18, 15, 48.5, 78]),
 dict(key="mode", name="MODE", kind="knob", x=854.1, y=289.6, ir=63.4, r=46, min=0, max=3, default=2, int=True, options=None, bw=100, base=136.8,
      angles=[45.7, 77.4, 106.4, 137.4]),
 # WAVEFORM switch: the dark tab above the saw / square drawing slides left (Saw) and right (Square). tab.box = the tab with its shadow in the
 # image (x0, y0, x1, y1); donor_x = an empty strip of the panel the same size that covers the tab's old spot; pos = tab centre x for Saw / Square.
 dict(key="waveform", name="WAVEFORM", kind="toggle", x=209, y=21, options=["Saw", "Square"], bw=96, bh=30,
      tab=dict(box=(188, 0, 230, 14), donor_x=234, pos=(188, 230))),
 # white keys (C ... C) and the sharps
 B("k_c", "C", 462, 658, 33, 70), B("k_d", "D", 572, 658, 33, 70), B("k_e", "E", 678, 658, 33, 70), B("k_f", "F", 782, 658, 33, 70),
 B("k_g", "G", 886, 658, 32, 70), B("k_a", "A", 988, 656, 35, 72), B("k_b", "B", 1092, 656, 34, 72), B("k_c2", "C'", 1198, 657, 35, 72),
 B("k_cs", "C#", 518, 531, 33, 72), B("k_ds", "D#", 626, 531, 34, 72), B("k_fs", "F#", 836, 531, 33, 72),
 B("k_gs", "G#", 937, 530, 36, 74), B("k_as", "A#", 1040, 528, 34, 73),
 # transpose / accent / slide
 B("k_down", "DOWN", 1305, 658, 32, 70), B("k_up", "UP", 1406, 657, 35, 72), B("k_accent", "ACCENT", 1510, 657, 35, 72), B("k_slide", "SLIDE", 1618, 658, 33, 70),
 # function, pitch mode, time mode, pattern clear, back, run/stop, tap
 B("k_function", "FUNCTION", 272, 656, 43, 81), B("k_pitch", "PITCH MODE", 312, 527, 75, 38), B("k_time", "TIME MODE", 1613, 503, 72, 36),
 B("k_clear", "PATTERN CLEAR", 122, 528, 83, 45), B("k_back", "BACK", 1751, 528, 80, 45),
 B("run", "RUN/STOP", 123, 656, 122, 81), B("k_tap", "TAP", 1752, 656, 117, 81),
 dict(key="brand", name="BRAND", kind="readout", cx=190, cy=684, w=300, h=56, tsize=34, talign="center", ql=False),
 dict(key="info", name="INFO", kind="readout", cx=640, cy=684, w=560, h=56, tsize=34, talign="center", ql=False),
 dict(key="status", name="STATUS", kind="readout", cx=1110, cy=684, w=340, h=56, tsize=26, talign="center", ql=False),
]
for c in C:
    if c.get("options", 0) is None: del c["options"]
SPEC = dict(name="303MPC", onepage=True, art=ART, pages=[("303", "")], controls=C,
            theme=dict(bg="1a1a1a", panel="1a1a1a", line="bfbfbf", ink="f2f2f2", ink_dim="c8c8c8", ink_faint="7a7a7a", accent="f07a1e",
                       accent_hi="ffffff", knob_face="151515", knob_ring="3c3c3c", knob_dot="f2f2f2", bar="101010", seg_active="f07a1e",
                       seg_inactive="151515", seg_active_tx="111111", btn_text="f2f2f2", tab_on="f07a1e", tab_on_tx="111111", lcd="101010", box="151515"))
