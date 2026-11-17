SHMPC 0.2.0 (pre-release, offline build only). Roland SH-101 emulation for Akai Force / MPC OS. Not tested on a Force or any MPC yet. New in 0.2.0: builds for the new-generation 64-bit MPCs and for MPC OS 3.10.1. Three zips: `-mpc-armv7.zip` (Gen1 MPCs and the Force, MPC OS 3.x, install.sh), `-mpc-aarch64.zip` (new-generation 64-bit MPCs, install.sh) and `-mpc-sdk.zip` (one folder for every MPC on MPC OS 3.10.1: turn on External Plugin Support in Preferences, Enhancements, and copy the folder into `VST2 Plugins` on a drive). The sound and panel are the same as 0.1.0. Not tried on any device.

Port by L'Cronx of the emulation by lemus-dev (https://github.com/lemus-dev/lemus), AGPL-3.0. No ROMs are included: bring your own.

Needs: the SH-101 ROM. Put it in the plugin's roms folder; it is found by SHA-1, so the file name does not matter. Without it the plugin runs in voice mode (MIDI plays the voice, the built-in sequencer is off).

Install: unzip, copy to the device, run `sh install.sh` as root (stops MPC, backs up MPC.settings, registers the plugin, restarts MPC), then add the plugin to a track. `sh uninstall.sh` removes it. The how-to is in docs/README.md inside the plugin folder.

Known limits: untested on hardware.
