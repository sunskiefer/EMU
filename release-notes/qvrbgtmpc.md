QVRBGT MPC 0.1.0 (pre-release, offline build only). Alesis QuadraVerb GT multi-effect emulation for Akai Force / MPC OS. Not tested on a Force or any MPC yet. Built for ARMv7 (Gen1 MPC and Force), MPC OS 3.x.

Port by L'Cronx of the emulation by lemus-dev (https://github.com/lemus-dev/lemus), AGPL-3.0. No ROMs are included: bring your own.

Needs: the firmware qvgtv103.bin (64 KB). Put it in the plugin's roms folder; it is found by SHA-1, so the file name does not matter. Without it the audio passes through unchanged and the display shows that the ROM is missing. Your programs are saved to qvgt-ram.bin in the same folder.

Install: unzip, copy to the device, run `sh install.sh` as root (stops MPC, backs up MPC.settings, registers the plugin, restarts MPC), then add the plugin to a track. `sh uninstall.sh` removes it. The how-to is in docs/README.md inside the plugin folder.

Known limits: placeholder panel art, CPU use unmeasured.
