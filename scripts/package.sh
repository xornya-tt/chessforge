#!/bin/sh
# Builds the muOS release files in out/:
#   ChessForge3D-manual.zip  unzip into the SD card root (recommended)
#   ChessForge3D.muxapp      for Archive Manager (experimental on MustardOS 2606.0)
# Usage: scripts/package.sh [build-dir] [stockfish-binary]
# Optional environment: STOCKFISH_COPYING = path to Stockfish's Copying.txt (shipped when Stockfish is included).
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="${1:-$ROOT/build-rg35xx}"
STOCKFISH="${2:-}"
OUT="$ROOT/out"
STAGE="$OUT/stage/ChessForge3D"

rm -rf "$OUT/stage" "$OUT/manual"
mkdir -p "$STAGE/bin" "$STAGE/saves" "$STAGE/glyph" "$STAGE/licenses"
cp "$BUILD_DIR/chessforge3d" "$STAGE/bin/chessforge3d"
cp "$ROOT/muos/mux_launch.sh" "$STAGE/mux_launch.sh"
cp "$ROOT/muos/glyph/icon.png" "$STAGE/glyph/icon.png"
cp "$ROOT/muos/input.cfg.example" "$STAGE/input.cfg.example"
cp "$ROOT/themes/neon-carbon.cfg" "$STAGE/neon-carbon.cfg.example"
cp "$ROOT/LICENSE" "$STAGE/licenses/ChessForge-LICENSE.txt"
cp "$ROOT/THIRD_PARTY.md" "$STAGE/licenses/THIRD_PARTY.md"
if [ -d "$ROOT/pieces" ]; then cp -r "$ROOT/pieces" "$STAGE/pieces"; fi
if [ -n "$STOCKFISH" ] && [ -f "$STOCKFISH" ]; then
    cp "$STOCKFISH" "$STAGE/bin/stockfish"
    if [ -n "${STOCKFISH_COPYING:-}" ] && [ -f "$STOCKFISH_COPYING" ]; then
        cp "$STOCKFISH_COPYING" "$STAGE/licenses/Stockfish-Copying.txt"
    else
        echo "warning: Stockfish is included but STOCKFISH_COPYING is not set" >&2
    fi
fi
chmod +x "$STAGE/mux_launch.sh" "$STAGE/bin/"*

# Manual install: unzip into the root of the SD card.
mkdir -p "$OUT/manual/MUOS/application"
cp -r "$STAGE" "$OUT/manual/MUOS/application/ChessForge3D"
(cd "$OUT/manual" && rm -f "$OUT/ChessForge3D-manual.zip" && zip -X -qr "$OUT/ChessForge3D-manual.zip" MUOS)

# Archive Manager package (stored without compression, with a checksum manifest).
(cd "$STAGE" && find . -type f ! -name manifest.sha256 | sed 's|^\./||' | LC_ALL=C sort | while IFS= read -r f; do sha256sum "$f"; done > manifest.sha256)
(cd "$STAGE" && rm -f "$OUT/ChessForge3D.muxapp" && zip -0 -X -qr "$OUT/ChessForge3D.muxapp" .)

echo "Ready:"
echo "  $OUT/ChessForge3D-manual.zip  -> unzip into the SD card root"
echo "  $OUT/ChessForge3D.muxapp      -> <SD>/MUOS/ARCHIVE/ (Archive Manager, experimental)"
