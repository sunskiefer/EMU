# 303MPC: ONE page over the plugin's own panel art (art/panels/303.png, 1201 x 547, as supplied).
# x, y are PIXELS OF THAT IMAGE. gen_emu.py turns them into page coordinates:  cx = 16 + x*1.0384,  cy = y*1.0384 + 86.
# To move a control, edit its x/y here and rebuild. ir = the knob's radius in the image; r = its radius on the page (also names
# its filmstrip, so every different-looking knob needs its own r). bw/bh = touch box on the page.
ART = dict(file="art/panels/303.png", scale=568 / 547, x=round((1280 - 1201 * 568 / 547) / 2), y=0)
K = lambda key, name, x, y, ir, r, d=5, base=0: dict(key=key, name=name, kind="knob", x=x, y=y, ir=ir, r=r, min=0, max=10, default=d, bw=84, base=base)
B = lambda key, name, x, y, bw=64, bh=70: dict(key=key, name=name, kind="button", x=x, y=y, bw=bw, bh=bh)
C = [
 K("tuning", "TUNING", 361, 63, 26, 27), K("cutoff", "CUT OFF", 452, 63, 26, 27), K("resonance", "RES", 541, 63, 26, 27),
 K("envmod", "ENV MOD", 632, 63, 26, 27), K("decay", "DECAY", 721, 63, 26, 27), K("accent", "ACCENT", 811, 63, 26, 27),
 K("tempo", "TEMPO", 180, 209, 43, 45, 5, -23), K("volume", "VOLUME", 1054, 209, 43, 46, 7, 57),
 dict(key="track", name="PATT GROUP", kind="knob", x=362, y=207, ir=42, r=44, min=0, max=6, default=0, int=True, options=None, bw=100, base=75, angles=[-102, -79, -46, -25, 25, 46, 79]),
 dict(key="mode", name="MODE", kind="knob", x=542, y=207, ir=42, r=47, min=0, max=3, default=2, int=True, options=None, bw=100, base=105, angles=[44, 71, 114, 137]),
 dict(key="waveform", name="WAVEFORM", kind="toggle", x=149, y=17, options=["Saw", "Square"], bw=96),
 B("k_c", "C", 302, 447), B("k_d", "D", 367, 443), B("k_e", "E", 433, 443), B("k_f", "F", 498, 443),
 B("k_g", "G", 564, 447), B("k_a", "A", 629, 443), B("k_b", "B", 695, 443), B("k_c2", "C'", 761, 443),
 B("k_cs", "C#", 334, 367, 48, 60), B("k_ds", "D#", 400, 367, 48, 60), B("k_fs", "F#", 531, 367, 48, 60),
 B("k_gs", "G#", 597, 367, 48, 60), B("k_as", "A#", 663, 367, 48, 60),
 B("k_down", "DOWN", 830, 443, 56), B("k_up", "UP", 895, 443, 56), B("k_accent", "ACCENT", 961, 443, 56), B("k_slide", "SLIDE", 1027, 443, 56),
 B("k_function", "FUNCTION", 182, 443, 56), B("k_pitch", "PITCH MODE", 207, 364, 80, 50), B("k_time", "TIME MODE", 1023, 349, 80, 50),
 B("k_clear", "PATTERN CLEAR", 88, 364, 80, 50), B("k_back", "BACK", 1121, 364, 80, 50),
 B("run", "RUN/STOP", 88, 443, 80, 60), B("k_tap", "TAP", 1121, 443, 80, 60),
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
