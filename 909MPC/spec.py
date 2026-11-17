# 909MPC: ONE page over the plugin's own panel art (art/panels/909.png, 872 x 399, as supplied).
# x, y are PIXELS OF THAT IMAGE (cx = ART.x + x*scale, cy = y*scale + 86 on the page). Move a control by editing its x/y here.
# ir = the knob's radius in the image, r = its radius on the page (also names its filmstrip). base = where the pointer is in the art
# (the knob's default). bw/bh = touch box on the page.
ART = dict(file="art/panels/909.png", scale=568 / 399, x=round((1280 - 872 * 568 / 399) / 2), y=0)
def K(key, name, x, y, d=5, base=0, ir=11, r=16):
    return dict(key=key, name=name, kind="knob", x=x, y=y, ir=ir, r=r, min=0, max=10, default=d, bw=60, base=base)
LV = 73   # the level knobs rest at 7.5 in the art (pointer at about 73 degrees)
C = [
 K("accent", "TOTAL ACCENT", 90, 92, 5, -6),
 K("bd_tune", "BD TUNE", 150, 52, 5, -6), K("bd_level", "BD LEVEL", 191, 52, 7.5, LV), K("bd_attack", "BD ATTACK", 150, 92, 5, -6), K("bd_decay", "BD DECAY", 192, 92, 5, -6),
 K("sd_tune", "SD TUNE", 241, 52, 5, 0), K("sd_level", "SD LEVEL", 282, 52, 7.5, LV), K("sd_tone", "SD TONE", 241, 92, 5, 6), K("sd_snappy", "SD SNAPPY", 283, 92, 5, -6),
 K("lt_tune", "LT TUNE", 332, 52, 5, 6), K("lt_level", "LT LEVEL", 374, 52, 7.5, LV), K("lt_decay", "LT DECAY", 333, 92, 5, -5),
 K("mt_tune", "MT TUNE", 424, 52, 5, 0), K("mt_level", "MT LEVEL", 465, 52, 7.5, LV), K("mt_decay", "MT DECAY", 424, 92, 5, -6),
 K("ht_tune", "HT TUNE", 515, 52, 5, -6), K("ht_level", "HT LEVEL", 556, 52, 7.5, LV), K("ht_decay", "HT DECAY", 515, 92, 5, -6),
 K("rs_level", "RS LEVEL", 605, 52, 7.5, LV), K("hc_level", "HC LEVEL", 647, 52, 7.5, LV),
 K("hh_level", "HH LEVEL", 739, 52, 7.5, LV), K("ch_decay", "CH DECAY", 697, 92, 5, 0), K("oh_decay", "OH DECAY", 739, 92, 5, 0),
 K("cr_level", "CRASH LEVEL", 788, 52, 7.5, LV), K("rd_level", "RIDE LEVEL", 830, 52, 7.5, LV),
 K("cr_tune", "CRASH TUNE", 789, 92, 5, -6), K("rd_tune", "RIDE TUNE", 831, 92, 5, 0),
 K("tempo", "TEMPO", 302, 168, 3.3, -49, ir=20, r=30),
 K("volume", "VOLUME", 828, 168, 7, 62, ir=20, r=30),
]
for c in C:
    if c["key"] in ("tempo", "volume"): c["bw"] = 90
B = lambda key, name, x, y, bw=52, bh=52: dict(key=key, name=name, kind="button", x=x, y=y, bw=bw, bh=bh)
C += [
 B("k_start", "START", 85, 169, 62, 62), B("k_stopcont", "STOP/CONT", 135, 169, 62, 62),
 B("k_trk1", "TRACK PLAY 1", 369, 159, 44, 46), B("k_trk2", "TRACK PLAY 2", 393, 159, 44, 46), B("k_trk3", "TRACK PLAY 3", 417, 159, 44, 46), B("k_trk4", "TRACK PLAY 4", 441, 159, 44, 46),
 B("k_pat1", "PATTERN PLAY 1", 476, 159, 44, 46), B("k_pat2", "PATTERN PLAY 2", 500, 159, 44, 46), B("k_pat3", "PATTERN PLAY 3", 524, 159, 44, 46), B("k_ext", "EXT INST", 547, 159, 44, 46),
 B("k_tempo", "TEMPO/STEP", 582, 159, 44, 46), B("k_back", "BACK/TAP", 607, 159, 44, 46), B("k_fwd", "FWD", 631, 159, 44, 46), B("k_meas", "AVAILABLE MEAS", 654, 159, 44, 46),
 B("k_cycle", "CYCLE/GUIDE", 682, 159, 44, 46), B("k_tape", "TAPE SYNC", 706, 159, 44, 46), B("k_enter", "CARTRIDGE/ENTER", 751, 169, 62, 62),
 B("k_last", "LAST STEP", 75, 225, 44, 44), B("k_scale", "SCALE", 105, 225, 44, 44), B("k_shuffle", "SHUFFLE/FLAM", 75, 261, 44, 44), B("k_clear", "CLEAR", 105, 261, 44, 44),
 B("k_inst", "INSTRUMENT SELECT", 75, 296, 44, 44), B("k_shift", "SHIFT", 105, 296, 44, 44),
]
SX = [148, 194, 239, 285, 331, 376, 422, 468, 513, 559, 605, 650, 695, 741, 787, 833]
for i, x in enumerate(SX): C.append(B("k_step%d" % (i + 1), "STEP %d" % (i + 1), x, 295, 56, 66))
C += [
 dict(key="brand", name="BRAND", kind="readout", cx=190, cy=684, w=300, h=56, tsize=34, talign="center", ql=False),
 dict(key="info", name="INFO", kind="readout", cx=640, cy=684, w=560, h=56, tsize=34, talign="center", ql=False),
 dict(key="status", name="STATUS", kind="readout", cx=1110, cy=684, w=340, h=56, tsize=26, talign="center", ql=False),
]
for _i, _k in enumerate(["bd_tune", "bd_level", "sd_tune", "sd_level", "lt_tune", "mt_tune", "ht_tune", "hh_level", "ch_decay", "oh_decay",
                         "cr_level", "rd_level", "tempo", "accent", "volume", "bd_decay"]):
    next(c for c in C if c["key"] == _k)["qlrank"] = _i
SPEC = dict(name="909MPC", onepage=True, art=ART, pages=[("909", "")], controls=C,
            theme=dict(bg="1a1a1a", panel="1a1a1a", line="bfbfbf", ink="f2f2f2", ink_dim="c8c8c8", ink_faint="7a7a7a", accent="f07a1e",
                       accent_hi="ffffff", knob_face="151515", knob_ring="3c3c3c", knob_dot="f2f2f2", bar="101010", seg_active="f07a1e",
                       seg_inactive="151515", seg_active_tx="111111", btn_text="f2f2f2", tab_on="f07a1e", tab_on_tx="111111", lcd="101010", box="151515"))
