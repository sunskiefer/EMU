# Vendored Lemus cores

Source: https://github.com/lemus-dev/lemus (https://lemusdev.org), commit 390273a97a15cbf3b28df2a7d6c1d3507c5f31af.
Licence: GNU AGPL-3.0 (see /LICENSE). Copyright the Lemus authors (lemus-dev).

Copied unchanged from `machines/<id>/core/` (the JUCE-free emulation cores) plus each machine's README:
`tb303` (Roland TB-303), `sh101` (Roland SH-101), `tr808` (Roland TR-808), `tr909` (Roland TR-909), `qvgt` (Alesis QuadraVerb GT).
The Virus C core is not used (its DSP56300 emulator needs a JIT that does not exist on ARMv7).

Roland, TB-303, SH-101, TR-808, TR-909, Alesis and QuadraVerb are trademarks of their owners. They are used only to say what is
being emulated. No firmware ROMs are included: you supply your own (see the README for the files and their SHA-1 sums).

## Local changes (the only ones; everything else is unchanged)
- `tr808/core/circuit.cpp`, `tr909/core/circuit.cpp`: the `Pair`/`pairLoad`/... helpers had an x86 (SSE2) and an aarch64 (NEON) version; an `#elif` x86 guard
  and an `#else` scalar two-double fallback were added so the code builds for ARMv7 (which has no double-precision SIMD). Same arithmetic, element by element.
  `<emmintrin.h>` is now included only on x86.
- `tr808/core/parallel.cpp`, `tr909/core/parallel.cpp`: the spin-wait pause is the ARM `yield` instruction on 32-bit ARM.
- `core/netlists.h` is not vendored (Lemus generates it from `core/netlists/*.cir` at build time): `tools/gen_netlists.py` writes it, and an include
  shim, into `<plugin>/build/inc/` (called from the plugin's `prebuild.sh`).
