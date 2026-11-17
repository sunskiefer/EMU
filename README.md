# EMU
Multiple emulation plugins for the Akai Force / MPC OS (ARMv7 VST2). **Bring your own ROMs.**

Emulations of classic studio machines from **Lemus** (https://github.com/lemus-dev/lemus, https://lemusdev.org), by lemus-dev,
ported by L'Cronx. All the emulation work is the Lemus authors'; this repo only wraps their cores as Force plugins.

| Plugin | Emulates | Kind | Status |
|---|---|---|---|
| 303MPC | Roland TB-303 | synth | one-page skin + engine built; ARM binary compiles; **not yet tested on a Force** |
| SHMPC | Roland SH-101 | synth | not built yet |
| 808MPC | Roland TR-808 | drum machine | not built yet |
| 909MPC | Roland TR-909 | drum machine | not built yet |
| QVRBGT MPC | Alesis QuadraVerb GT | effect | not built yet |

## ROMs
No ROMs are shipped. Put the file in the plugin's `roms` folder (it is found by its SHA-1, the name does not matter).
Without it the 303MPC runs in "voice mode" (MIDI plays the analog voice, the built-in sequencer is off).

| File | Machine | SHA-1 |
|---|---|---|
| `d650c-133.ic8` (2048 bytes) | TB-303 | 7a4236b2bc9a5cd4c80c63ca1a193e03becfcb4c |
| `qvgtv103.bin` (64 KB) | QuadraVerb GT firmware | 0ac063e74b6c6ecf9edda10824df9a79c621ccd9 |
| `hd44780_a00.bin` (4 KB) | QuadraVerb GT display font | 65cf075a988cdcbb316b9afdd0529b374a1a65ec |

## Panel art
Each plugin is ONE page over its own panel picture (`art/panels/`). Every control sits at its position in the picture; all
positions are in the plugin's `spec.py` (pixels of the picture) and `layout.conf`. Knobs turn in place (filmstrips cut from the
picture's own knobs). The bottom band shows "L'Cronx", the name and value of the control you touched, and the machine status.

## Build (no Docker)
`./build.sh 303MPC` (needs python3 + Pillow + opencv, a host gcc, `pip install ziglang`, and a checkout of
https://github.com/sd88me/mpc-vst-plugins as `../mpc-vst-plugins` or MPC_VST=...). Output: `303MPC/build/package/`.

## Licence
AGPL-3.0 (see LICENSE), the same as Lemus. Lemus cores are vendored unchanged under `vendor/lemus/` (see VENDORED.md).
