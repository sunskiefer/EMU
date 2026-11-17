Packaged release zips, offline builds only, not tested on any MPC or Force.
0.1.0: ARMv7 only (Gen1 MPCs and the Force). Published as GitHub pre-releases.
0.2.0: each plugin in three zips, made with the open-mpc release tools: -mpc-armv7.zip (Gen1 MPCs and the Force, install.sh),
-mpc-aarch64.zip (new-generation 64-bit MPCs, install.sh) and -mpc-sdk.zip (one folder for every MPC on MPC OS 3.10.1, copy into VST2 Plugins).
0.2.0 is NOT a published GitHub release or catalog entry yet.
Build: ./build.sh <plugin> for ARMv7, ARCH=aarch64 ./build.sh <plugin> for the 64-bit build.
