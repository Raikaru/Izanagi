#!/bin/sh
# Run on the Linux build host: appimage_smoke.sh <AppImage> <private ISO> <evidence-dir>
# Only the package, the private ISO and this script enter a fresh baseline container.
set -eu
if [ "${1:-}" = --inside ]; then
  export DEBIAN_FRONTEND=noninteractive
  apt-get update -qq
  apt-get install -y --no-install-recommends libegl1 libgles2 libgl1-mesa-dri >/evidence/mesa-install.log
  export HOME=/evidence/home XDG_DATA_HOME=/evidence/home/.local/share
  export P4PC_INPUT=script SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy
  mkdir -p "$XDG_DATA_HOME/p4pc/Persona4"
  cd /evidence
  # No supplied disc: exercise the no-argument first-run path, never a fake ELF.
  rc=0
  P4PC_CONFIG=/evidence/first.ini timeout 30 /package.AppImage --appimage-extract-and-run > /evidence/first-run.log 2>&1 || rc=$?
  [ "$rc" = 1 ] && grep -q 'No disc image' /evidence/first-run.log
  echo 'first-run: requests own disc image PASS'
  # Supply the player's selected ISO through the existing scripted first-run seam.
  P4PC_ISO=/disc.iso P4PC_CONFIG=/evidence/settings.ini P4PC_SAVE_DIR=/evidence/save \
    P4PC_TRAP_CONTINUE=1 P4PC_TURBO=1 P4PC_SETTINGS_TEST=1 \
    P4PC_MENU_SCRIPT='settings-title-ready,shot:title,settings-open,wait240,settings-check:title,settings-key:Escape,wait90,shot:title-closed' \
    P4PC_MENU_EXIT=1 \
    timeout 300 /package.AppImage --appimage-extract-and-run > /evidence/title.log 2>&1
  grep -q 'real title selector is ready' /evidence/title.log
  grep -q 'script finished:.*PASS' /evidence/title.log
  grep -q 'read SLUS_217.82 from the disc' /evidence/title.log
  echo 'AppImage: extracted own-disc executable, real title offscreen, exit=0 PASS'
  exit 0
fi
image=$(realpath "${1:?AppImage required}")
iso=$(realpath "${2:?private ISO required}")
mkdir -p "${3:?evidence directory required}"
evidence=$(realpath "$3")
engine=${P4PC_ENGINE:-docker}
"$engine" run --rm --network=bridge --cpus=2 --security-opt label=disable \
  -v "$image:/package.AppImage:ro" -v "$iso:/disc.iso:ro" \
  -v "$evidence:/evidence" -v "$(realpath "$0"):/smoke.sh:ro" \
  debian:stable-slim sh /smoke.sh --inside
