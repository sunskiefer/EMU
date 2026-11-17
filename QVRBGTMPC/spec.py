# QVRBGT MPC (Alesis QuadraVerb GT emulation, an effect): ONE page over art/panels/qvgt.png, 1180 x 568.
# THE PANEL ART IS A PLACEHOLDER (tools/make_qvgt_panel.py): no QuadraVerb GT panel image has been supplied. Replace art/panels/qvgt.png with the
# real one and move the x/y below onto it; nothing else changes. x, y are PIXELS OF THE IMAGE (cx = ART.x + x*scale, cy = y*scale + 86 on the page).
ART = dict(file="art/panels/qvgt.png", scale=1.0, x=50, y=0)
KEYS = [("k_program", "PROGRAM"), ("k_eq", "EQ"), ("k_preamp", "PRE-AMP"), ("k_pitch", "PITCH"), ("k_delay", "DELAY"), ("k_reverb", "REVERB"), ("k_mix", "MIX"), ("k_config", "CONFIG"),
        ("k_valdn", "VALUE -"), ("k_valup", "VALUE +"), ("k_pgdn", "PAGE -"), ("k_pgup", "PAGE +"), ("k_store", "STORE"), ("k_namemod", "NAME/MOD"), ("k_midi", "MIDI"), ("k_bypass", "BYPASS")]
C = [
 dict(key="input", name="INPUT", kind="knob", x=895, y=150, ir=54, r=59, min=0, max=10, default=10, bw=130, base=0, lit=(225, 225, 233)),
 dict(key="output", name="OUTPUT", kind="knob", x=1060, y=150, ir=54, r=59, min=0, max=10, default=10, bw=130, base=0, lit=(225, 225, 233)),
]
for i, (k, n) in enumerate(KEYS):
    C.append(dict(key=k, name=n, kind="button", x=123 + 137 * (i % 8), y=330 if i < 8 else 450, bw=130, bh=88))
C += [
 dict(key="lcd1", name="LCD LINE 1", kind="readout", cx=460, cy=196, w=680, h=70, tsize=56, talign="center", tcolor="b6f0a0", tfont="Roboto", tweight="Bold", ql=False),
 dict(key="lcd2", name="LCD LINE 2", kind="readout", cx=460, cy=276, w=680, h=70, tsize=56, talign="center", tcolor="b6f0a0", tfont="Roboto", tweight="Bold", ql=False),
 dict(key="brand", name="BRAND", kind="readout", cx=190, cy=684, w=300, h=56, tsize=34, talign="center", ql=False),
 dict(key="info", name="INFO", kind="readout", cx=640, cy=684, w=560, h=56, tsize=34, talign="center", ql=False),
 dict(key="status", name="STATUS", kind="readout", cx=1110, cy=684, w=340, h=56, tsize=26, talign="center", ql=False),
]
for _i, _k in enumerate(["input", "output"]):
    next(c for c in C if c["key"] == _k)["qlrank"] = _i
SPEC = dict(name="QVRBGT MPC", onepage=True, art=ART, pages=[("QVGT", "")], controls=C,
            theme=dict(bg="1a1a1a", panel="1a1a1a", line="bfbfbf", ink="f2f2f2", ink_dim="c8c8c8", ink_faint="7a7a7a", accent="f07a1e",
                       accent_hi="ffffff", knob_face="151515", knob_ring="3c3c3c", knob_dot="f2f2f2", bar="101010", seg_active="f07a1e",
                       seg_inactive="151515", seg_active_tx="111111", btn_text="f2f2f2", tab_on="f07a1e", tab_on_tx="111111", lcd="101010", box="151515"))
