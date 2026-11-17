# SHMPC: ONE page over the plugin's own panel art (art/panels/101.png, 952 x 467, as supplied).
# x, y are PIXELS OF THAT IMAGE (cx = ART.x + x*scale, cy = y*scale + 86 on the page). Move a control by editing its x/y here.
# Sliders and switches: cap = the lever cap's width/height in the image; y0 = cap centre at the bottom (value 0), y1 = at the top (max);
# a switch lists its positions (cap centre y) in pos. bw/bh = touch box on the page.
ART = dict(file="art/panels/101.png", scale=568 / 467, x=round((1280 - 952 * 568 / 467) / 2), y=0)
def SL(key, name, x, d=0.0, y0=122, y1=69):
    return dict(key=key, name=name, kind="slider_v", x=x, y=(y0 + y1) / 2, w=26, h=(y0 - y1) + 44, cap=(16, 33), y0=y0, y1=y1,
                min=0, max=10, default=d, bw=40)
def SW(key, name, x, pos, d=0, cap=(13, 34)):
    return dict(key=key, name=name, kind="slider_v", x=x, y=(pos[0] + pos[-1]) / 2, w=26, h=(pos[-1] - pos[0]) + 44, cap=cap,
                pos=pos, min=0, max=len(pos) - 1, default=d, int=True, bw=40)
def KN(key, name, x, y, ir, r, d=5, base=0, a0=-145, a1=145, mx=10, angles=None, integer=False):
    c = dict(key=key, name=name, kind="knob", x=x, y=y, ir=ir, r=r, min=0, max=mx, default=d, bw=80, base=base, a0=a0, a1=a1)
    if angles: c["angles"] = angles
    if integer: c["int"] = True
    return c
B = lambda key, name, x, y, bw=64, bh=70: dict(key=key, name=name, kind="button", x=x, y=y, bw=bw, bh=bh)
C = [
 KN("tune", "TUNE", 63, 97, 22, 28, 5, 10), KN("volume", "VOLUME", 41, 277, 22, 29, 7, 45), KN("portamento", "PORTAMENTO", 120, 280, 22, 30, 0, -146),
 KN("lfowave", "LFO WAVE", 169, 97, 17, 22, 0, -40, mx=3, integer=True, angles=[-100, -35, 35, 80]),
 KN("range", "RANGE", 272, 97, 17, 23, 1, -30, mx=3, integer=True, angles=[-75, -25, 25, 75]),
 SL("lforate", "LFO RATE", 119, 5), SL("vcomod", "VCO MOD", 223.5), SL("pulsewidth", "PULSE WIDTH", 321.5),
 SW("pwm", "PWM SOURCE", 355.5, [79, 95, 111], 1),
 SL("pulselevel", "PULSE", 404.5), SL("sawlevel", "SAW", 431, 10), SL("sublevel", "SUB", 457, 4), SW("submode", "SUB OSC", 490.5, [76, 94, 112], 1),
 SL("noiselevel", "NOISE", 537),
 SL("vcffreq", "FREQ", 580.5, 3.5), SL("vcfres", "RES", 606.5, 3), SL("vcfenv", "ENV", 645, 5), SL("vcfmod", "MOD", 671.5), SL("vcfkybd", "KYBD", 698, 5),
 SW("vca", "VCA", 751, [91, 106], 0), SW("trigger", "TRIGGER", 798, [87, 100, 113], 0),
 SL("attack", "ATTACK", 830.5), SL("decay", "DECAY", 857.5, 4), SL("sustain", "SUSTAIN", 884.5, 3), SL("release", "RELEASE", 910.5, 2),
 SL("bendvco", "BEND VCO", 28, 2, 377, 342), SL("bendvcf", "BEND VCF", 54, 0, 377, 342), SL("lfomod", "BEND LFO", 79, 5, 377, 342),
 dict(key="portamode", name="PORTAMENTO MODE", kind="button", x=118, y=340, bw=80, bh=30, cycle=True),
 dict(key="transpose", name="TRANSPOSE", kind="button", x=122, y=378, bw=80, bh=30, cycle=True),
 B("b_load", "LOAD", 178, 167, 56, 60), B("b_play", "PLAY", 207, 167, 56, 60), B("b_down", "ARP DOWN", 246, 167, 56, 60),
 B("b_ud", "ARP UP/DOWN", 274, 167, 56, 60), B("b_up", "ARP UP", 302, 167, 56, 60), B("b_hold", "HOLD", 341, 167, 56, 60),
 B("b_keytr", "KEY TRANSPOSE", 380, 167, 56, 60),
]
# the 32 keys, F3 (key 0) .. C6 (key 31): 19 white keys across x 155..912
WH = [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19, 21, 23, 24, 26, 28, 30, 31]
BL = [1, 3, 5, 8, 10, 13, 15, 17, 20, 22, 25, 27, 29]
KW = (912 - 155) / 19
NAMES = ["F", "F#", "G", "G#", "A", "A#", "B", "C", "C#", "D", "D#", "E"]
for n in WH: C.append(B("key_%d" % n, "KEY %s" % NAMES[n % 12], round(155 + KW * (WH.index(n) + .5)), 425, 46, 92))
for n in BL: C.append(B("key_%d" % n, "KEY %s" % NAMES[n % 12], round(155 + KW * (WH.index(n - 1) + 1)), 300, 30, 110))
C += [
 dict(key="brand", name="BRAND", kind="readout", cx=190, cy=684, w=300, h=56, tsize=34, talign="center", ql=False),
 dict(key="info", name="INFO", kind="readout", cx=640, cy=684, w=560, h=56, tsize=34, talign="center", ql=False),
 dict(key="status", name="STATUS", kind="readout", cx=1110, cy=684, w=340, h=56, tsize=26, talign="center", ql=False),
]
for _i, _k in enumerate(['vcffreq', 'vcfres', 'vcfenv', 'vcfmod', 'attack', 'decay', 'sustain', 'release', 'volume', 'tune', 'lforate', 'vcomod', 'pulselevel', 'sawlevel', 'sublevel', 'noiselevel']):
    next(c for c in C if c["key"] == _k)["qlrank"] = _i
SPEC = dict(name="SHMPC", onepage=True, art=ART, pages=[("SH", "")], controls=C,
            theme=dict(bg="1a1a1a", panel="1a1a1a", line="bfbfbf", ink="f2f2f2", ink_dim="c8c8c8", ink_faint="7a7a7a", accent="f07a1e",
                       accent_hi="ffffff", knob_face="151515", knob_ring="3c3c3c", knob_dot="f2f2f2", bar="101010", seg_active="f07a1e",
                       seg_inactive="151515", seg_active_tx="111111", btn_text="f2f2f2", tab_on="f07a1e", tab_on_tx="111111", lcd="101010", box="151515"))
