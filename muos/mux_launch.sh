#!/bin/sh
# ChessForge 3D — muOS application launcher.
# Lives in <SD>/MUOS/application/ChessForge3D/ and is started from Applications.

[ -f /opt/muos/script/var/func.sh ] && . /opt/muos/script/var/func.sh

APP_DIR="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
cd "$APP_DIR" || exit 1
mkdir -p saves

chmod +x bin/chessforge3d bin/stockfish 2>/dev/null

# Optional bundled libraries and controller mapping database.
export LD_LIBRARY_PATH="$APP_DIR/lib:${LD_LIBRARY_PATH:-}"
for db in /opt/muos/device/current/control/gamecontrollerdb.txt \
          /usr/lib/gamecontrollerdb.txt \
          /usr/share/gamecontrollerdb.txt; do
    if [ -f "$db" ]; then
        export SDL_GAMECONTROLLERCONFIG_FILE="$db"
        break
    fi
done
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-alsa}"

./bin/chessforge3d >"saves/run.log" 2>&1
