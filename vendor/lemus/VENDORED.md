# Vendored Lemus cores

Source: https://github.com/lemus-dev/lemus (https://lemusdev.org), commit 390273a97a15cbf3b28df2a7d6c1d3507c5f31af.
Licence: GNU AGPL-3.0 (see /LICENSE). Copyright the Lemus authors (lemus-dev).

Copied unchanged from `machines/<id>/core/` (the JUCE-free emulation cores) plus each machine's README:
`tb303` (Roland TB-303), `sh101` (Roland SH-101), `tr808` (Roland TR-808), `tr909` (Roland TR-909), `qvgt` (Alesis QuadraVerb GT).
The Virus C core is not used (its DSP56300 emulator needs a JIT that does not exist on ARMv7).

Roland, TB-303, SH-101, TR-808, TR-909, Alesis and QuadraVerb are trademarks of their owners. They are used only to say what is
being emulated. No firmware ROMs are included: you supply your own (see the README for the files and their SHA-1 sums).
