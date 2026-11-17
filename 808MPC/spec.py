# 808MPC: ONE page over the plugin's own panel art (art/panels/808.png, 881 x 483, as supplied).
# x, y are PIXELS OF THAT IMAGE (cx = ART.x + x*scale, cy = y*scale + 86 on the page). Move a control by editing its x/y here.
# ir = the knob's radius in the image, r = its radius on the page (also names its filmstrip, so a differently-looking knob needs its own r).
# base = where the pointer is in the art (its resting value). bw/bh = touch box on the page.
ART = dict(file="art/panels/808.png", scale=568 / 483, x=round((1280 - 881 * 568 / 483) / 2), y=0)
def K(key, name, x, y, d=5, base=0, ir=11, r=16, **kw):
    c = dict(key=key, name=name, kind="knob", x=x, y=y, ir=ir, r=r, min=0, max=10, default=d, bw=60, base=base, a0=-150, a1=150)
    c.update(kw); return c
LV = 75   # the level knobs rest at their printed dot, 7.5 (pointer at +75 degrees)
C = [
 K("ac_level", "ACCENT", 226, 41, 7.5, LV, r=15),
 K("bd_level", "BD LEVEL", 280, 41, 7.5, LV, r=15), K("bd_tone", "BD TONE", 281, 103), K("bd_decay", "BD DECAY", 282, 162),
 K("sd_level", "SD LEVEL", 334, 41, 7.5, LV, r=15), K("sd_tone", "SD TONE", 335, 103), K("sd_snappy", "SD SNAPPY", 335, 163),
 K("lt_level", "LT/LC LEVEL", 389, 43, 7.5, LV, r=15), K("lt_tuning", "LT/LC TUNING", 389, 103),
 K("mt_level", "MT/MC LEVEL", 442, 43, 7.5, LV, r=15), K("mt_tuning", "MT/MC TUNING", 442, 103),
 K("ht_level", "HT/HC LEVEL", 496, 43, 7.5, LV, r=15), K("ht_tuning", "HT/HC TUNING", 496, 103),
 K("rs_level", "RS/CL LEVEL", 550, 43, 7.5, LV, r=15), K("cp_level", "CP/MA LEVEL", 604, 43, 7.5, LV, r=15),
 K("cb_level", "CB LEVEL", 658, 43, 7.5, LV, r=15),
 K("cy_level", "CY LEVEL", 712, 43, 7.5, LV, r=15), K("cy_tone", "CY TONE", 713, 101), K("cy_decay", "CY DECAY", 713, 162),
 K("oh_level", "OH LEVEL", 766, 43, 7.5, LV, r=15), K("oh_decay", "OH DECAY", 766, 162),
 K("ch_level", "CH LEVEL", 820, 43, 7.5, LV, r=15),
 K("volume", "MASTER VOLUME", 812, 255, 7, 58, ir=20, r=25, bw=80),
 K("tempo", "TEMPO", 80, 226, 5, 42, ir=37, r=45, bw=100),
 K("fine", "FINE", 165, 248, 5, 0, ir=12, r=14, bw=56),
 dict(key="mode", name="MODE", kind="knob", x=70, y=67, ir=19, r=24, min=0, max=5, default=3, int=True, bw=80, base=15, angles=[-75, -45, -15, 15, 45, 75]),
 dict(key="instrument", name="INSTRUMENT", kind="knob", x=157, y=69, ir=19, r=23, min=0, max=11, default=0, int=True, bw=80, base=-165,
      angles=[-165 + 30 * i for i in range(12)]),
 dict(key="autofill", name="AUTO FILL IN", kind="knob", x=157, y=145, ir=19, r=22, min=0, max=5, default=0, int=True, bw=80, base=-75, angles=[-75, -45, -15, 15, 45, 75]),
]
for c in C:
    if c["kind"] == "knob" and c.get("int"): c.pop("a0", None); c.pop("a1", None)
TG = lambda key, name, x, y, o: dict(key=key, name=name, kind="toggle", x=x, y=y, options=o, bw=56, bh=56)
C += [
 TG("lc", "LOW CONGA", 391, 181, ["TOM", "CONGA"]), TG("mc", "MID CONGA", 442, 181, ["TOM", "CONGA"]), TG("hc", "HI CONGA", 496, 183, ["TOM", "CONGA"]),
 TG("cl", "CLAVES", 549, 183, ["RIM SHOT", "CLAVES"]), TG("ma", "MARACAS", 604, 183, ["HAND CLAP", "MARACAS"]),
 TG("ifvar", "I/F VARIATION", 812, 322, ["A", "B"]),
]
B = lambda key, name, x, y, bw=50, bh=50, **kw: dict(key=key, name=name, kind="button", x=x, y=y, bw=bw, bh=bh, **kw)
C += [
 B("variation", "BASIC VARIATION", 88, 322, 90, 44, cycle=True), B("prescale", "PRE-SCALE", 168, 333, 50, 70, cycle=True),
 B("k_startstop", "START/STOP", 88, 398, 100, 70), B("k_tap", "TAP", 812, 399, 80, 70), B("k_clear", "CLEAR", 70, 121, 44, 44),
]
for i in range(16): C.append(B("k_step%d" % (i + 1), "STEP %d" % (i + 1), round(213 + 36.07 * i), 397, 36, 64))
C += [
 dict(key="brand", name="BRAND", kind="readout", cx=190, cy=684, w=300, h=56, tsize=34, talign="center", ql=False),
 dict(key="info", name="INFO", kind="readout", cx=640, cy=684, w=560, h=56, tsize=34, talign="center", ql=False),
 dict(key="status", name="STATUS", kind="readout", cx=1110, cy=684, w=340, h=56, tsize=26, talign="center", ql=False),
]
for _i, _k in enumerate(["ac_level", "bd_level", "bd_tone", "bd_decay", "sd_level", "sd_tone", "sd_snappy", "lt_level", "lt_tuning", "mt_level",
                         "mt_tuning", "ht_level", "ht_tuning", "cy_level", "volume", "tempo"]):
    next(c for c in C if c["key"] == _k)["qlrank"] = _i
SPEC = dict(name="808MPC", onepage=True, art=ART, pages=[("808", "")], controls=C,
            theme=dict(bg="1a1a1a", panel="1a1a1a", line="bfbfbf", ink="f2f2f2", ink_dim="c8c8c8", ink_faint="7a7a7a", accent="f07a1e",
                       accent_hi="ffffff", knob_face="151515", knob_ring="3c3c3c", knob_dot="f2f2f2", bar="101010", seg_active="f07a1e",
                       seg_inactive="151515", seg_active_tx="111111", btn_text="f2f2f2", tab_on="f07a1e", tab_on_tx="111111", lcd="101010", box="151515"))
