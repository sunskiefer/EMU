SHMPC 0.1.0 (pre-release, offline build only). Roland SH-101 emulation for Akai Force / MPC OS. Not tested on a Force or any MPC yet. Built for ARMv7 (Gen1 MPC and Force), MPC OS 3.x.

Port by L'Cronx of the emulation by lemus-dev (https://github.com/lemus-dev/lemus), AGPL-3.0. No ROMs are included: bring your own.

Needs: the SH-101 ROM. Put it in the plugin's roms folder; it is found by SHA-1, so the file name does not matter. Without it the plugin runs in voice mode (MIDI plays the voice, the built-in sequencer is off).

Install: unzip, copy to the device, run `sh install.sh` as root (stops MPC, backs up MPC.settings, registers the plugin, restarts MPC), then add the plugin to a track. `sh uninstall.sh` removes it. The how-to is in docs/README.md inside the plugin folder.

Known limits: untested on hardware.
