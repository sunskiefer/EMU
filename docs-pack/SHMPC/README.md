# SHMPC: Roland SH-101 emulation (synth) for Akai Force / MPC OS

Version 0.1.0. An AGPL-3.0 port by L'Cronx of the emulation by lemus-dev (https://github.com/lemus-dev/lemus). Source: https://github.com/sunskiefer/EMU

**Status: offline build only. It has not been tested on a Force or any MPC.** Real-time CPU use is unmeasured.

No ROMs are included. Bring your own.

# How to use

1. Requirements: Akai Force or MPC (ARMv7) on MPC OS 3.x, root SSH access.
2. Install: unzip, copy the folder to the device, run `sh install.sh` as root. It stops MPC, backs up MPC.settings, copies the plugin to /sdcard/Synths, registers it and restarts MPC. `sh uninstall.sh` removes it.
3. Your ROM: put the SH-101 ROM, SHA-1 a84bb1758582dbb800001db3663eaf3fb9d0e25d in the plugin's `roms` folder (`/sdcard/Synths/L'Cronx - VST - <name>/roms/`) (it is found by its SHA-1, the file name does not matter). Restart the plugin after adding it. Without the ROM it runs in voice mode: MIDI plays the voice and the built-in sequencer is off.
4. Use: add the plugin to a track from the plugin list. Play the voice from a MIDI track or keyboard; sliders and buttons are on the one panel page.
5. The panel is one page. Touch a control and the bottom band shows its name and value. Q-Links follow the controls.
6. Uninstall: `sh uninstall.sh` (your files in `roms` are kept).
