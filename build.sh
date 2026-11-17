#!/usr/bin/env bash
# Build one EMU plugin for the Force / MPC OS (armv7-a hard-float, glibc 2.31 so it also loads on MPC OS 2.x).
#   ./build.sh 303MPC            (MPC_VST=../mpc-vst-plugins to use another framework checkout)
# Needs: python3 + Pillow, a host gcc (for shadow_art) and Zig (pip install ziglang). No Docker.
# Output: <plugin>/build/package/<vendor> - VST - <name>/ (skin + .so) and <plugin>/build/pluginlist-entry.xml
set -euo pipefail
cd "$(dirname "$0")"
PLUGIN="${1:?usage: build.sh <plugin folder, e.g. 303MPC>}"
MPC_VST="${MPC_VST:-$PWD/../mpc-vst-plugins}"
ZIG="${ZIG:-python3 -m ziglang}"
cd "$PLUGIN"
python3 ../tools/gen_emu.py spec.py
[ -x prebuild.sh ] && ./prebuild.sh   # e.g. the 808/909 netlist header
mkdir -p build/host build/arm build/arm64
[ -x build/host/shadow_art ] || gcc -O2 -w -I"$MPC_VST/tools/vendor/force-shadow/tools" -x c -o build/host/shadow_art "$MPC_VST/tools/shadow_art.c" -lm
rm -rf build/skin
SHADOW_ART="$PWD/build/host/shadow_art" GEN_VST_RELOCK="${GEN_VST_RELOCK:-}" python3 "$MPC_VST/tools/gen_vst.py" vst.json
SKINDIR="build/skin/$(python3 -c "import json;d=json.load(open('vst.json'));print(d['vendor']+' - VST - '+d['name'])")/Plugin Skins"
if python3 -c "import runpy,sys;sys.exit(0 if runpy.run_path('spec.py')['SPEC'].get('onepage') else 1)"; then python3 ../tools/post_art.py "$SKINDIR" layout.conf params.json spec.py; else python3 ../tools/post_skin.py "build/skin/$(python3 -c "import json;d=json.load(open('vst.json'));print(d['vendor']+' - VST - '+d['name'])")/Plugin Skins" layout.conf params.json; fi
# the vendor name has an apostrophe (L'Cronx): read every name from vst.json, always quoted
VENDOR=$(python3 -c "import json;print(json.load(open('vst.json'))['vendor'])")
NAME=$(python3 -c "import json;print(json.load(open('vst.json'))['name'])")
SO=$(python3 -c "import json;print(json.load(open('vst.json'))['so'])")
FOLDER="$VENDOR - VST - $NAME"
SRCS=$(python3 -c "import json; print(' '.join(json.load(open('vst.json'))['build']['sources']))")
CFLAGS=$(python3 -c "import json; print(' '.join(json.load(open('vst.json'))['build']['cflags']))")
ARCH="${ARCH:-armv7}"   # armv7 (Gen1 MPC and Force) or aarch64 (Gen2 MPCs, 64-bit)
if [ "$ARCH" = aarch64 ]; then
  TGT="-target aarch64-linux-gnu.2.31 -mcpu=generic -ffp-contract=off"; ADIR=build/arm64
else
  TGT="-target arm-linux-gnueabihf.2.31 -mcpu=generic+v7a+vfp3d16-d32-neon+thumb2 -ffp-contract=off"; ADIR=build/arm
fi
COMMON="-O2 -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections -DNDEBUG -w"
OBJS=""
for f in $SRCS; do
  o="$ADIR/$(echo "$f" | tr / _).o"
  case "$f" in
    *.cpp|*.cc) (cd .. && $ZIG c++ $TGT $COMMON -std=gnu++17 -fno-rtti -I"$PLUGIN" -Icommon -I"$PLUGIN/build" -I"$MPC_VST/wrapper" $CFLAGS -c "$f" -o "$PLUGIN/$o") ;;
    *) (cd .. && $ZIG cc $TGT $COMMON -std=gnu11 -I"$PLUGIN/build" -c "$f" -o "$PLUGIN/$o") ;;
  esac
  OBJS="$OBJS $o"
done
# defines from vst.json are in build/params.h; the wrapper is plain C
$ZIG cc $TGT $COMMON -std=gnu11 -Ibuild -c "$MPC_VST/wrapper/vst2_wrap.c" -o "$ADIR/vst2_wrap.o"
printf '{\n  global: VSTPluginMain;\n  local: *;\n};\n' > "$ADIR/exports.map"
$ZIG c++ $TGT -shared -fPIC -Wl,--no-undefined -Wl,--version-script="$ADIR/exports.map" -Wl,--gc-sections -Wl,-s \
  $OBJS "$ADIR/vst2_wrap.o" -lm -ldl -lpthread -o "$ADIR/$SO"
PKG="build/package/$FOLDER"
[ "$ARCH" = armv7 ] && rm -rf build/package; mkdir -p "$PKG"
[ -d "build/skin/$FOLDER" ] && cp -r "build/skin/$FOLDER/." "$PKG/"
cp "$ADIR/$SO" "$PKG/$( [ "$ARCH" = aarch64 ] && echo "${SO%.so}-aarch64.so" || echo "$SO")"
mkdir -p "$PKG/roms"
echo "highest glibc: $(readelf -V "$ADIR/$SO" | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1)"
echo "-> $PWD/$PKG"
